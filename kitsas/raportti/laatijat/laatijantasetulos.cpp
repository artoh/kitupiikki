#include "laatijantasetulos.h"

#include "db/kirjanpito.h"

#include <QRegularExpression>
#include <QRegularExpressionMatch>

#include "../raporttirivi.h"

#include "db/kirjanpito.h"
#include "db/tilikausi.h"

#include <QJsonDocument>

LaatijanTaseTulos::LaatijanTaseTulos(RaportinLaatija *laatija, const RaporttiValinnat &valinnat) :
    LaatijanRaportti(laatija, valinnat)
{

}

void LaatijanTaseTulos::laadi()
{
    QString muoto = valinnat().arvo(RaporttiValinnat::RaportinMuoto).toString();
    tyyppi_ = valinnat().arvo(RaporttiValinnat::Tyyppi).toString();

    if( muoto.isEmpty()) {
        muoto = tyyppi_ == "tase" ? "tase/yleinen" : "tulos/yleinen";
    }

    const QString kaava = kp()->asetukset()->asetus(muoto);
    QJsonDocument doc = QJsonDocument::fromJson( kaava.toUtf8());
    kmap_ = doc.toVariant().toMap();

    rk.asetaOtsikko( kmap_.value("nimi").toMap().value(kielikoodi()).toString() );
    if( tyyppi_ == "kohdennus")
        rk.asetaKausiteksti(kaanna("Kustannuspaikat"));
    else if( tyyppi_ == "projektit")
        rk.asetaKausiteksti(kaanna("Projektit"));

    erittelyt_ = valinnat().onko(RaporttiValinnat::TulostaErittely);

    kirjoitaYlatunniste();

    // Kohdennuspoiminnan otsikko
    int kohdennuksella = valinnat().arvo(RaporttiValinnat::Kohdennuksella).toInt();
    if(kohdennuksella > -1)
        rk.asetaOtsikko( rk.otsikko() + "(" + kp()->kohdennukset()->kohdennus(kohdennuksella).nimi(kielikoodi()) + ")");

    // Datan tilaaminen
    QList<KpKysely*> kyselyt;

    // Tase
    if( tyyppi_ == "tase") {
        for(const RaporttiValintaSarake& sarake : valinnat().sarakkeet()  ) {
            tilauslaskuri_++;
            KpKysely* kysely = kpk("/saldot");
            kysely->lisaaAttribuutti("pvm", sarake.pvm());
            kysely->lisaaAttribuutti("tase");
            int sarakeid = ++sarakemaara_ - 1;
            connect( kysely, &KpKysely::vastaus, this,
                     [this, sarakeid] (QVariant* vastaus) { this->dataSaapuu(sarakeid, vastaus); });
            kyselyt.append(kysely);
        }
    } else {
        // Muut
        for(const RaporttiValintaSarake& sarake : valinnat().sarakkeet()) {

            if( sarake.tyyppi() != RaporttiValintaSarake::Budjetti) {
                tilauslaskuri_++;
                KpKysely *kysely = kpk("/saldot");
                kysely->lisaaAttribuutti("alkupvm", sarake.alkuPvm());
                kysely->lisaaAttribuutti("pvm", sarake.loppuPvm());
                if( kohdennuksella > -1)
                    kysely->lisaaAttribuutti("kohdennus", kohdennuksella);

                if( tyyppi_ == "kohdennus")
                    kysely->lisaaAttribuutti("kustannuspaikat");
                else if( tyyppi_ == "projektit")
                    kysely->lisaaAttribuutti("projektit");
                else
                    kysely->lisaaAttribuutti("tuloslaskelma");

                int sarakeid = ++sarakemaara_ - 1;
                connect(kysely, &KpKysely::vastaus, this,
                        [this, sarakeid] (QVariant* vastaus) { this->dataSaapuu(sarakeid, vastaus);} );
                kyselyt.append(kysely);
            }

            if( sarake.tyyppi() != RaporttiValintaSarake::Toteutunut) {
                tilauslaskuri_++;
                KpKysely* kysely = kpk( QString("/budjetti/%1").arg(sarake.alkuPvm().toString(Qt::ISODate)));
                if( kohdennuksella > -1)
                    kysely->lisaaAttribuutti("kohdennus", kohdennuksella);
                if( tyyppi_ == "kohdennus" || tyyppi_ == "projektit")
                    kysely->lisaaAttribuutti("kohdennukset");
                int sarakeid = ++sarakemaara_ - 1;
                connect(kysely, &KpKysely::vastaus, this,
                        [this, sarakeid] (QVariant* vastaus) {this->dataSaapuu(sarakeid, vastaus);});
                kyselyt.append(kysely);
            }

        }
    }
    for(auto kysely : kyselyt) {
        kysely->kysy();
    }
}

QString LaatijanTaseTulos::nimi() const
{
    return kmap_.value("nimi").toMap().value(kielikoodi()).toString();
}

void LaatijanTaseTulos::dataSaapuu(int sarake, QVariant *variant)
{
    QVariantMap map = variant->toMap();

    if( tyyppi_ == "kohdennus" || tyyppi_ == "projektit") {
        // Jos haetaan kohdennusraportti, sijoitetaan kohdennukset omiin
        // senttitaulukoihinsa jotka sitten puretaan kohdennus kerrallaan

        QMapIterator<QString,QVariant> kiter(map);
        while(kiter.hasNext()) {
            kiter.next();

            int kohdennus = kiter.key().toInt();
            QHash<int, QVector<Euro> > eurot = kohdennetut_.value(kohdennus);

            QMapIterator<QString, QVariant> iter( kiter.value().toMap());
            while( iter.hasNext()) {
                iter.next();
                int tili = iter.key().toInt();
                Euro euro = Euro(iter.value().toString());
                if( euro == Euro::Zero) continue;

                if( !eurot.contains(tili))
                    eurot.insert(tili, QVector<Euro>(sarakemaara_));
                eurot[tili][sarake] = euro;
            }
            kohdennetut_.insert( kohdennus, eurot);
        }
    } else {
        QMapIterator<QString, QVariant> iter(map);
        while( iter.hasNext()) {
            iter.next();
            int tili = iter.key().toInt();
            QString euroStr = iter.value().toString();
            Euro euro = Euro(euroStr);
            if( euro == Euro::Zero) continue;    // Nollaeuroja ei huomioida

            if( !eurot_.contains(tili))
                eurot_.insert(tili, QVector<Euro>(sarakemaara_));
            eurot_[tili][sarake] = euro;
        }
    }

    tilauslaskuri_--;

    if( !tilauslaskuri_) {

        if( !kohdennetut_.isEmpty()) {
            // Puretaan kohdennuslaskelmaa
            QMapIterator<int, QHash<int, QVector<Euro>>> kkiter( kohdennetut_);
            while( kkiter.hasNext()) {
                kkiter.next();
                eurot_ = kkiter.value();

                Kohdennus kohdennus = kp()->kohdennukset()->kohdennus( kkiter.key());
                RaporttiRivi korivi;
                korivi.lisaa( kohdennus.nimi(kielikoodi()), 2 );
                korivi.asetaKoko(14);
                rk.lisaaRivi(korivi);

                if( kohdennus.tyyppi() == Kohdennus::PROJEKTI) {
                    RaporttiRivi kprivi;
                    Kohdennus paikka = kp()->kohdennukset()->kohdennus( kohdennus.kuuluu() );
                    kprivi.lisaa( paikka.nimi(kielikoodi()), 2 );
                    rk.lisaaRivi(kprivi);
                }
                rk.lisaaTyhjaRivi();
                kirjoitaRaportti();
                rk.lisaaTyhjaRivi();
            }
        } else {
            kirjoitaRaportti();
        }
        valmis();
    }

}

void LaatijanTaseTulos::kirjoitaRaportti()
{
    laadiTililista();
    QRegularExpression tiliRe("(?<alku>\\d{1,8})(\\.\\.)?(?<loppu>\\d{0,8})");

    QVector<Euro> kokosumma(sarakemaara_);

    QVariantList rivilista = kmap_.value("rivit").toList();
    bool edellinenOliValisumma = false;

    for(const QVariant& riviVariant : rivilista) {
        const QVariantMap map = riviVariant.toMap();

        const QString kaava = map.value("L").toString();
        const int sisennys = map.value("S").toInt();
        const QString teksti = map.value(kielikoodi()).toString();

        const QString muoto = map.value("M").toString();

        for(int i=0; i < map.value("V").toInt(); i++)
            rk.lisaaTyhjaRivi();

        RaporttiRivi rr;
        if( muoto.contains("bold"))
            rr.lihavoi();
        rr.sisenna(sisennys);
        rr.lisaa(teksti);

        if( kaava.isEmpty()) {
            // Vain otsikko
            rk.lisaaRivi(rr);
            continue;
        }

        const bool otsikkoRivi = kaava.contains('h', Qt::CaseInsensitive);
        const bool naytaTyhjarivi = kaava.contains('S') || kaava.contains('H');
        const bool laskeValisummaan = !kaava.contains("==");
        const bool lisaaValisumma = kaava.contains("=") && laskeValisummaan;
        const bool naytaErittelyt = kaava.contains('*');
        const bool vainMenot = kaava.contains('-');
        const bool vainTulot = kaava.contains('+');

        if(lisaaValisumma && edellinenOliValisumma) {
            continue;   // Ei kahta välisummaa peräkkäin
        }

        QList<int> rivinTilit;  // Tilit, jotka kuuluvat tälle välille

        QRegularExpressionMatchIterator ri = tiliRe.globalMatch(kaava );

        while( ri.hasNext())
        {
            QRegularExpressionMatch tiliMats = ri.next();
            QString alku = tiliMats.captured("alku");
            QString loppu = tiliMats.captured("loppu");

            if( loppu.isEmpty())
                loppu = alku;

            int alkumerkit = alku.length();
            int loppumerkit = loppu.length();

            // Sitten etsitään tilit listalle
            for(const QString& tili : qAsConst( tilit_)) {
                if( tili.left(alkumerkit) >= alku && tili.left(loppumerkit) <= loppu )
                    rivinTilit.append(tili.toInt());
            }

        }

        // Lasketaan summiin
        QVector<Euro> summa(sarakemaara_);

        for(int tili : rivinTilit) {
            if( vainMenot || vainTulot) {
                const Tili& tamaTili = kp()->tilit()->tiliNumerolla(tili);
                if( (vainMenot && !tamaTili.onko(TiliLaji::MENO)) ||
                    (vainTulot && !tamaTili.onko(TiliLaji::TULO)))
                    continue;
            }
            for(int i=0; i < sarakemaara_; i++)
                summa[i] += eurot_.value(tili, QVector<Euro>(sarakemaara_))[i];
        }

        if( laskeValisummaan && !otsikkoRivi) {
            for(int i=0; i < sarakemaara_; i++)
                kokosumma[i] += summa[i];
        }

        if( lisaaValisumma ) {
            for(int i=0; i < sarakemaara_; i++)
                summa[i] += kokosumma[i];
        } else if( !naytaTyhjarivi && rivinTilit.isEmpty()) {
            continue;
        }

        edellinenOliValisumma = lisaaValisumma;

        if( !otsikkoRivi) {
            int taulukkoindeksi = 0;
            for(const auto sarake : valinnat().sarakkeet()) {
                switch (sarake.tyyppi()) {
                case RaporttiValintaSarake::Toteutunut:
                    rr.lisaa( summa[taulukkoindeksi], true );
                    break;
                case RaporttiValintaSarake::Budjetti:
                    rr.lisaa( summa[taulukkoindeksi], true);
                    break;
                case RaporttiValintaSarake::BudjettiEro:
                    rr.lisaa( summa[taulukkoindeksi] - summa[taulukkoindeksi+1], true);
                    taulukkoindeksi++;
                    break;
                case RaporttiValintaSarake::ToteumaProsentti:
                    const Euro toteutunut = summa[taulukkoindeksi];
                    const Euro budjetoitu = summa[taulukkoindeksi + 1];
                    if( budjetoitu.cents() == 0)
                        rr.lisaa("");
                    else
                        rr.lisaa( 10000 * toteutunut / budjetoitu, true);
                    taulukkoindeksi++;
                }
                taulukkoindeksi++;
            }
        }
        rk.lisaaRivi(rr);

        if( naytaErittelyt && erittelyt_) {


            for( int tilinumero : rivinTilit) {
                const Tili* tili = kp()->tilit()->tili(tilinumero);

                if( !tili ||
                    (vainMenot && !tili->onko(TiliLaji::MENO)) ||
                    (vainTulot && !tili->onko(TiliLaji::TULO)))
                        continue;

                RaporttiRivi er;
                er.sisenna(rr.sisennys() + 1);
                er.lisaaLinkilla( RaporttiRiviSarake::TILI_NRO, tili->numero(), tili->nimiNumero(kielikoodi()));

                int taulukkoindeksi = 0;
                for(const auto sarake : valinnat().sarakkeet()) {
                    switch (sarake.tyyppi()) {
                    case RaporttiValintaSarake::Toteutunut:
                        er.lisaa( eurot_.value(tilinumero).value(taulukkoindeksi), true );
                        break;
                    case RaporttiValintaSarake::Budjetti:
                        er.lisaa( eurot_.value(tilinumero).value(taulukkoindeksi), false);
                        break;
                    case RaporttiValintaSarake::BudjettiEro:
                        er.lisaa( eurot_.value(tilinumero).value(taulukkoindeksi) - eurot_.value(tilinumero).value(taulukkoindeksi+1), true);
                        taulukkoindeksi++;
                        break;
                    case RaporttiValintaSarake::ToteumaProsentti:
                        const Euro toteutunut = eurot_.value(tilinumero).value(taulukkoindeksi);
                        const Euro budjetoitu = eurot_.value(tilinumero).value(taulukkoindeksi+1);
                        if( budjetoitu.cents() == 0)
                            er.lisaa("");
                        else
                            er.lisaa( 10000 * toteutunut / budjetoitu, true);
                        taulukkoindeksi++;
                    }
                    taulukkoindeksi++;
                }
                rk.lisaaRivi(er);
            }  // Tili
        }   // Erittelyt

    } // Rivin käsittely


}

void LaatijanTaseTulos::laadiTililista()
{
    tilit_.clear();
    tilit_.reserve( eurot_.count() + 1 );
    for(const int tilinumero : eurot_.keys()) {
        tilit_.append(QString::number(tilinumero));
    }
    tilit_.sort();
}

void LaatijanTaseTulos::kirjoitaYlatunniste()
{
    bool erikoissarakkeita = false;
    rk.lisaaVenyvaSarake();

    for(const auto& sarake : valinnat().sarakkeet()) {
        if( sarake.tyyppi() != RaporttiValintaSarake::Toteutunut)
            erikoissarakkeita = true;
        rk.lisaaEurosarake();
    }
    if( tyyppi_ != "tase") {
        RaporttiRivi csvrivi(RaporttiRivi::CSV);
        csvrivi.lisaa("");

        for(const auto& sarake : valinnat().sarakkeet()) {
            const QString tyyppiTeksti = erikoissarakkeita ? sarakeTyyppiTeksti(sarake.tyyppi()) : QString() ;
            csvrivi.lisaa( QString("%1 - %2 %3").arg( sarake.alkuPvm().toString("dd.MM.yyyy"))
                                              .arg( sarake.loppuPvm().toString("dd.MM.yyyy"))
                                              .arg( tyyppiTeksti ), 1, true );
        }
        rk.lisaaOtsake(csvrivi);

        RaporttiRivi orivi(RaporttiRivi::EICSV);
        orivi.lisaa("");
        for(const auto& sarake : valinnat().sarakkeet()) {
            orivi.lisaa( QString("%1 -").arg( sarake.alkuPvm().toString("dd.MM.yyyy") ), 1, true );
        }
        rk.lisaaOtsake(orivi);
    }

    // Tasepäivät tai loppupäivät
    RaporttiRivi olrivi(RaporttiRivi::KAIKKI);
    olrivi.lisaa("");

    for(const auto& sarake : valinnat().sarakkeet()) {
        olrivi.lisaa( sarake.loppuPvm().toString("dd.MM.yyyy"), 1, true );
    }
    rk.lisaaOtsake(olrivi);


    if( erikoissarakkeita )
    {
        RaporttiRivi tyyppirivi(RaporttiRivi::KAIKKI);
        tyyppirivi.lisaa("");
        for(const auto& sarake : valinnat().sarakkeet()) {
            tyyppirivi.lisaa( sarakeTyyppiTeksti(sarake.tyyppi()), 1, true );
        }
        rk.lisaaOtsake( tyyppirivi);
    }

}

QString LaatijanTaseTulos::sarakeTyyppiTeksti(const RaporttiValintaSarake::SarakeTyyppi &tyyppi) const
{
    switch (tyyppi) {
        case RaporttiValintaSarake::Toteutunut: return kaanna("Toteutunut");
        case RaporttiValintaSarake::Budjetti: return kaanna("Budjetti");
        case RaporttiValintaSarake::BudjettiEro: return kaanna("Budjettiero €");
        case RaporttiValintaSarake::ToteumaProsentti: return kaanna("Toteutunut %");
    }
    return QString();
}
