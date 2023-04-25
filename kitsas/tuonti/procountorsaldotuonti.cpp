#include "procountorsaldotuonti.h"

#include "csvtuonti.h"
#include "tilimuuntomodel.h"
#include "db/tositetyyppimodel.h"
#include "model/tositevienti.h"

ProcountorSaldoTuonti::ProcountorSaldoTuonti()
{

}

QVariantMap ProcountorSaldoTuonti::tuo(const QByteArray &data)
{
    QList<QStringList> csv = Tuonti::CsvTuonti::csvListana(data);


    TiliMuuntoModel muunto;

    QList<QDate> paivat;

    for(const QStringList& rivi : qAsConst(csv)) {
        if( rivi.length() < 2) continue;

        if( rivi.length() > 2 && ProcountorSaldoTuonti::kkPaivaksi(rivi.at(1)).isValid()) {
            QList<QDate>  haettuPaivaLista;
            for(int i=1; i < rivi.length(); i++) {
                QDate paiva = ProcountorSaldoTuonti::kkPaivaksi(rivi.at(i));
                if( !paiva.isValid())
                    break;
                haettuPaivaLista << paiva;
            }
            if( haettuPaivaLista.length() > paivat.length()) {
                paivat = haettuPaivaLista;
            }
            continue;
        }

        bool saldoja = false;
        QList<Euro> saldot;

        for(int i=1; i < rivi.length() && i <= paivat.length(); i++) {
            Euro saldo = Euro::fromString(rivi.at(i));
            if( saldo ) saldoja = true;
            saldot << saldo;
        }

        const QString tilistr = rivi.value(0);

        if( !saldoja ) continue;
        QRegularExpressionMatch mats = tiliRE__.match(tilistr);
        if( mats.hasMatch()) {
            const int tili = mats.captured(1).toInt();
            const QString nimi = mats.captured(2);

            muunto.lisaa(tili, nimi, saldot);
        }
    }

    if( muunto.rowCount() < 3 || paivat.isEmpty()) {
        return QVariantMap();
    }

    muunto.asetaSaldoPaivat(paivat);
    if( muunto.naytaMuuntoDialogi( qApp->focusWidget() ) == QDialog::Accepted) {

        QVariantList viennit;

        for(int i=0; i < muunto.rowCount(); i++) {
            const int tiliNro = muunto.tilinumeroIndeksilla(i);
            Tili* tili = kp()->tilit()->tili(tiliNro);
            if(tili && !tili->onko(TiliLaji::KAUDENTULOS)) {

                for(const auto& era : muunto.eraIndeksilla(i)) {
                    TositeVienti vienti;
                    vienti.setPvm(era.pvm());
                    vienti.setTili(tiliNro);
                    Euro saldo = era.saldo();
                    if( saldo == Euro::Zero) continue;
                    bool debet = tili->onko(TiliLaji::VASTATTAVAA) ^ (saldo < Euro::Zero);
                    if(debet)
                        vienti.setDebet(saldo.abs());
                    else
                        vienti.setKredit(saldo.abs());
                    viennit.append(vienti);
                }
            }
        }

        QVariantMap map;
        map.insert("tyyppi", TositeTyyppi::TUONTI);
        map.insert("pvm", paivat.last()  );
        map.insert("viennit", viennit);
        return map;
    }
    return QVariantMap();

}

QDate ProcountorSaldoTuonti::kkPaivaksi(const QString teksti)
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

QRegularExpression ProcountorSaldoTuonti::tiliRE__ = QRegularExpression(R"(\D*(\d{3,8})\W*(.+))", QRegularExpression::UseUnicodePropertiesOption);
