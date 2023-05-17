#include "procountortuontitiedosto.h"
#include "tuonti/csvtuonti.h"

#include <QFile>

#include "db/kirjanpito.h"
#include "model/tositevienti.h"
#include "model/tositeviennit.h"

#include "procountortuontidialog.h"

ProcountorTuontiTiedosto::ProcountorTuontiTiedosto()
{

}

ProcountorTuontiTiedosto::TuontiStatus ProcountorTuontiTiedosto::tuo(const QString &polku)
{
    QFile file(polku);

    if( !file.open( QIODevice::ReadOnly | QIODevice::Text)) {
        return TIEDOSTO_VIRHE;
    }

    QByteArray data = file.readAll();
    file.close();

    QList<QStringList> csv = Tuonti::CsvTuonti::csvListana(data);

    tyyppi_ = TASE_UUSI;

    for(const QStringList& rivi : qAsConst(csv)) {
        if( rivi.length() < 7 ) continue;

        // Haetaan päivämäärälista
        if(paivat_.isEmpty() && paivaksi(rivi.at(1)).isValid()) {
            QList<QDate> paivalista;
            for(int i=1; i < rivi.length(); i++) {
                QDate paiva = paivaksi(rivi.at(i));
                if( !paiva.isValid() || paivalista.contains(paiva))
                    break;
                paivalista << paiva;
            }
            if( paivalista.length() > 5) {
                paivat_ = paivalista;
            }
            continue;
        }

        const QString tilistr = rivi.value(0);
        QRegularExpressionMatch mats = tiliRE__.match(tilistr);
        if( mats.hasMatch()) {
            const QString tili = mats.captured(1);
            const QString nimi = mats.captured(2);
            QList<Euro> saldot;

            for(int i=1; i < rivi.length() && i <= paivat_.length(); i++) {
                saldot << Euro::fromString(rivi.at(i));
            }
            SaldoTieto tieto = SaldoTieto(tili, nimi, saldot);

            if( tili > '3') tyyppi_ = TULOSLASKELMA;
            if( tili.startsWith("2") && nimi.contains("Edellisten tilikausien") && tieto.summa())
                tyyppi_ = TASE;  // Tase on avattu, koska siinä edellisten tilikausien tulos

            saldot_.append(tieto);
        }
    }

    if( paivat_.isEmpty()) return PAIVAT_PUUTTUU;

    // Tsekataan tilikaudet
    const TilikausiModel* kaudet = kp()->tilikaudet();
    if( kaudet->rowCount() < 2) return ProcountorTuontiTiedosto::KAUDET_PUUTTUU;

    if( paivat_.last() == kaudet->tilikausiIndeksilla(0).paattyy()) {
        kausi_ = EDELLINEN;
        if( tyyppi_ == TASE_UUSI) tyyppi_ = TASE;
    } else if( paivat_.last() == kaudet->tilikausiIndeksilla(1).paattyy())
        kausi_ = TAMA;
    else {
        return VIRHEELLISET_KAUDET;
    }

    if( !validi())
        return TIEDOSTO_TUNTEMATON;

    return TUONTI_OK;
}

void ProcountorTuontiTiedosto::lisaaMuuntoon(TiliMuuntoModel *muunto)
{
    for(const auto& saldo : saldot_) {
        muunto->lisaa(saldo.tilinumero(), saldo.tilinimi(), saldo.summa());
    }
}

bool ProcountorTuontiTiedosto::validi()
{
    return tyyppi_ && kausi_;
}

void ProcountorTuontiTiedosto::tallennaTilinavaukseen(TilinavausModel *tilinavaus, TiliMuuntoModel *muunto)
{
    tilinavaus->setKuukausittain(true);

    for(const auto& saldo : saldot_) {
        const int muunnettu = muunto->muunnettu(saldo.tilinumero());
        if( !muunnettu) continue;

        QList<AvausEra> erat;
        QList<AvausEra> vanhatErat = tilinavaus->erat(muunnettu);

        for(int i=0; i < paivat_.count(); i++) {
            const QDate& pvm = paivat_.at(i);
            const Euro kkSaldo = saldo.saldot().value(i) + vanhatErat.value(i).saldo();

            AvausEra era(kkSaldo, pvm);
            erat << era;
        }

        tilinavaus->asetaErat(muunnettu, erat);
    }

}

void ProcountorTuontiTiedosto::oikaiseTilinavaus(const ProcountorTuontiTiedosto &edellinenTase)
{
    for(const auto& edellinenSaldo : edellinenTase.saldot_) {
        oikaiseTili(edellinenSaldo);
    }
}

void ProcountorTuontiTiedosto::oikaiseTili(const SaldoTieto &saldotieto)
{
    for(int i=0; i < saldot_.count(); i++) {
        if( saldotieto.tilinumero() == saldot_.at(i).tilinumero()) {
            saldot_[i].oikaiseAvauksesta(saldotieto.summa());
            return;
        }
    }
    // Tämä saldo on nollattu, eli nollaaminen on lisättävä saldoihin
    QList<Euro> nollaus;
    nollaus << Euro::Zero - saldotieto.summa();
    for(int i=1; i < paivat_.count(); i++)
        nollaus << Euro::Zero;
    saldot_.append(SaldoTieto(QString::number(saldotieto.tilinumero()), saldotieto.tilinimi(), nollaus));
}

void ProcountorTuontiTiedosto::tallennaAlkutositteeseen(Tosite *tosite, TiliMuuntoModel *muunto)
{
    for( const auto& saldo : saldot_) {
        const int muunnettu = muunto->muunnettu(saldo.tilinumero());
        if( !muunnettu ) continue;

        Tili* tili = kp()->tilit()->tili(muunnettu);
        if(!tili) continue;
        if(tili->onko(TiliLaji::KAUDENTULOS) || tili->onko(TiliLaji::EDELLISTENTULOS)) continue;

        for(int i=0; i < paivat_.count(); i++) {
            const Euro& maara = saldo.saldot().value(i);
            if( !maara ) continue;

            TositeVienti vienti;
            vienti.setPvm( paivat_.at(i) );
            vienti.setTili( muunnettu );
            vienti.setSelite( ProcountorTuontiDialog::tr("Procountorista %3 %1 %2").arg(saldo.tilinumero()).arg(saldo.tilinimi()).arg(paivat_.at(i).toString("MM/yyyy"))  );

            if( tili->onko(TiliLaji::VASTAAVAA) ^ (maara < Euro::Zero)) {
                vienti.setDebet(maara.abs());
            } else {
                vienti.setKredit(maara.abs());
            }
            tosite->viennit()->lisaa(vienti);
        }
    }
}

QDate ProcountorTuontiTiedosto::paivaksi(const QString &teksti)
{
    if( teksti.contains(" - ")) {
        QStringList osat = teksti.split(" - ");
        if( osat.count() == 2) {
            QDate loppu = QDate::fromString(osat.last(),"dd.MM.yyyy");
            if(loppu.isValid()) return loppu;
        }
    }

    QStringList patkina = teksti.split("/");
    if( patkina.length() == 2) {
        int kk = patkina.at(0).toInt();
        int vvvv = patkina.at(1).toInt();
        if( kk > 0 && kk < 13 && vvvv > 2010 && vvvv < 2200) {
            QDate eka(vvvv, kk, 1);
            return QDate(vvvv, kk, eka.daysInMonth());
        }
    }
    return QDate();
}

QRegularExpression ProcountorTuontiTiedosto::tiliRE__ = QRegularExpression(R"(\D*(\d{3,8})\W*(.+))", QRegularExpression::UseUnicodePropertiesOption);

ProcountorTuontiTiedosto::SaldoTieto::SaldoTieto()
{

}

ProcountorTuontiTiedosto::SaldoTieto::SaldoTieto(const QString& tilinumero, const QString &tilinimi, const QList<Euro> &saldot) :
    tilinumero_(tilinumero), tilinimi_(tilinimi), saldot_(saldot)
{

}

Euro ProcountorTuontiTiedosto::SaldoTieto::summa() const
{
    Euro yhteensa = Euro::Zero;
    for(const auto& euro : saldot_) {
        yhteensa += euro;
    }
    return yhteensa;
}

void ProcountorTuontiTiedosto::SaldoTieto::oikaiseAvauksesta(const Euro &euro)
{
    if( saldot_.empty()) return;
    saldot_[0] = saldot_.at(0) - euro;
}
