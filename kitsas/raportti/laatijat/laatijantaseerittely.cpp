#include "laatijantaseerittely.h"
#include "db/kirjanpito.h"
#include "laskutus/huoneisto/huoneistomodel.h"

LaatijanTaseErittely::LaatijanTaseErittely(RaportinLaatija *laatija, const RaporttiValinnat &valinnat) :
    LaatijanRaportti(laatija, valinnat)
{

}

void LaatijanTaseErittely::laadi()
{
    mista_ = valinnat().arvo(RaporttiValinnat::AlkuPvm).toDate();
    mihin_ = valinnat().arvo(RaporttiValinnat::LoppuPvm).toDate();
    piilotaNollat_ = valinnat().arvo(RaporttiValinnat::PiilotaNollasaldot).toBool();

    KpKysely* kysely = kpk("/erat/erittely");
    kysely->lisaaAttribuutti("alkaa", mista_);
    kysely->lisaaAttribuutti("loppuu", mihin_);
    connect( kysely, &KpKysely::vastaus, this, &LaatijanTaseErittely::dataSaapuu );
    kysely->kysy();
}

void LaatijanTaseErittely::dataSaapuu(QVariant *data)
{
    rk.asetaCsvKaytossa(false);
    rk.asetaOtsikko(kaanna("TASE-ERITTELY"));
    rk.asetaKausiteksti(QString("%1 - %2").arg(mista_.toString("dd.MM.yyyy"), mihin_.toString("dd.MM.yyyy")));

    rk.lisaaSarake("12345678"); // Tosite
    rk.lisaaPvmSarake();        // Päivämäärä
    rk.lisaaVenyvaSarake();     // Asiakas/toimittaja
    rk.lisaaVenyvaSarake();     // Selite
    rk.lisaaEurosarake();

    // Lisätään otsikot
    {
        RaporttiRivi yotsikko;
        yotsikko.lisaa(kaanna("Tili"),3);
        rk.lisaaOtsake(yotsikko);

        RaporttiRivi otsikko;
        otsikko.lisaa(kaanna("Tosite"));
        otsikko.lisaa(kaanna("Päivämäärä"));
        otsikko.lisaa(kaanna("Selite"));
        otsikko.lisaa("");
        otsikko.lisaa("€",1,true);
        rk.lisaaOtsake(otsikko);
        rk.lisaaRivi();
    }

    bool vastattavaa = false;
    RaporttiRivi otsikkoRivi;
    otsikkoRivi.lisaa(kaanna("VASTAAVAA"),3);
    otsikkoRivi.asetaKoko(14);
    rk.lisaaRivi(otsikkoRivi);

    QVariantMap map = data->toMap();

    // Lisätään ylimääräinen map lajittelua varten
    QMap<QString,QString> lajitteluMap;
    QMapIterator<QString,QVariant> lajitteluIter(map);
    while(lajitteluIter.hasNext()) {
        lajitteluIter.next();
        const QString& avain = lajitteluIter.key();
        lajitteluMap.insert( avain.left(avain.length()-1), avain);
    }
    QList<QString> lajiteltuLista;
    QMapIterator<QString, QString> lajiteltuIter(lajitteluMap);
    while( lajiteltuIter.hasNext()) {
        lajiteltuIter.next();
        lajiteltuLista.append(lajiteltuIter.value());
    }


    Euro yhteensa = Euro::Zero;

    for( const QString& koodi : lajiteltuLista) {
        QVariant tieto = map.value(koodi);

        if( koodi.startsWith('2') && !vastattavaa) {
            RaporttiRivi summaRivi;
            summaRivi.lisaa(kaanna("VASTAAVAA YHTEENSÄ"),4);
            summaRivi.lisaa(yhteensa);
            rk.lisaaRivi(summaRivi);
            rk.lisaaTyhjaRivi();
            yhteensa = Euro::Zero;

            RaporttiRivi otsikkoRivi;
            otsikkoRivi.lisaa(kaanna("VASTATTAVAA"),4);
            otsikkoRivi.asetaKoko(14);
            rk.lisaaRivi(otsikkoRivi);
            vastattavaa = true;
        }

        QChar tyyppi = koodi.at( koodi.length()-1 );
        int tilinumero = koodi.left(koodi.length()-1).toInt();
        Tili* tili = kp()->tilit()->tili(tilinumero);
        if( !tili)
            continue;

        if( tyyppi == 'S' ||
            (tyyppi == 'M' && tieto.toMap().value("kausi").toList().isEmpty())  ) {    // VAIN SALDO
            RaporttiRivi rr;
            Euro saldo = tyyppi == 'S' ? Euro::fromString(tieto.toString()) : Euro::fromString( tieto.toMap().value("saldo").toString() );
            rr.lisaaLinkilla( RaporttiRiviSarake::TILI_NRO, tili->numero(), tili->nimiNumeroIban(kielikoodi()), 4 );
            rr.lisaa( saldo, true);
            rr.lihavoi();
            if(saldo || !piilotaNollat_) rk.lisaaRivi(rr);
            yhteensa += saldo;
        } else {
            Euro tilinLoppusaldo = Euro::Zero;
            QList<RaporttiRivi> rivit;

            // Nimike
            RaporttiRivi tilinnimi;
            tilinnimi.lisaaLinkilla( RaporttiRiviSarake::TILI_NRO, tili->numero(), tili->nimiNumeroIban(kielikoodi()), 4 );
            tilinnimi.lihavoi();
            rivit.append(tilinnimi);

            if( tyyppi == 'T') {
                // Täysi tase-erittely
                rivit.append(RaporttiRivi());

                for(const QVariant& era : tieto.toList() ) {
                    QVariantMap map = era.toMap();

                    Euro saldo = Euro::fromVariant(map.value("saldo"));
                    if( piilotaNollat_ && !saldo) continue;

                    QVariantMap eramap = map.value("era").toMap();
                    Euro ennenKautta = Euro::fromVariant(map.value("ennen"));
                    Euro alkuSaldo = Euro::fromVariant(eramap.value("eur"));


                    RaporttiRivi nimirivi;

                    if( eramap.isEmpty()) {
                        nimirivi.lisaa("",2);
                        nimirivi.lisaa(kaanna("Erittelemättömät"),3);
                    } else {
                        lisaaTositeTunnus( &nimirivi, eramap);
                        nimirivi.lisaa( eramap.value("vientipvm").toDate() );

                        QString kumppani = eramap.value("kumppani").toString();
                        QString selite = eramap.value("selite").toString();

                        if(kumppani.isEmpty() || selite == kumppani) {
                            nimirivi.lisaa(selite, 2);
                        } else {
                            nimirivi.lisaa( kumppani);
                            nimirivi.lisaa( selite);
                        }

                        nimirivi.lisaa( Euro::fromVariant(eramap.value("eur")));
                    }
                    rivit.append(nimirivi);

                    if( (eramap.isEmpty() || eramap.value("pvm").toDate() < mista_ ) && ennenKautta && (ennenKautta - alkuSaldo) ) {
                        if( !eramap.isEmpty()) {
                            RaporttiRivi poistettuRivi;
                            poistettuRivi.lisaa(" ",2);
                            poistettuRivi.lisaa( kaanna("Lisäykset/vähennykset %1 saakka").arg( mista_.addDays(-1).toString("dd.MM.yyyy")),2);
                            poistettuRivi.lisaa( ennenKautta - alkuSaldo, true);
                            rivit.append(poistettuRivi);
                        }

                        RaporttiRivi saldorivi;
                        saldorivi.lisaa(" ", 2);
                        saldorivi.lisaa( kaanna("Jäljellä %1").arg( mista_.toString("dd.MM.yyyy")),2);
                        saldorivi.lisaa( ennenKautta, true );
                        saldorivi.viivaYlle();
                        rivit.append(saldorivi);
                    }

                    // Muutokset
                    for( const QVariant& muutos : map.value("kausi").toList()) {
                        QVariantMap mmap = muutos.toMap();
                        RaporttiRivi rr;
                        lisaaTositeTunnus(&rr, mmap);
                        rr.lisaa( mmap.value("vientipvm").toDate());
                        QString kumppani = mmap.value("kumppani").toString();
                        QString selite = mmap.value("selite").toString();

                        if(kumppani.isEmpty() || selite == kumppani) {
                            rr.lisaa(selite, 2);
                        } else {
                            rr.lisaa( kumppani);
                            rr.lisaa( selite);
                        }
                        rr.lisaa(Euro::fromVariant(map.value("eur")));
                        rivit.append(rr);
                    }
                    // Loppusaldo
                    RaporttiRivi loppuRivi;
                    loppuRivi.lisaa(" ", 2);
                    loppuRivi.lisaa( kaanna("Loppusaldo %1").arg(mihin_.toString("dd.MM.yyyy")),2);
                    loppuRivi.lisaa( saldo,true);
                    loppuRivi.viivaYlle();

                    rivit.append(loppuRivi);
                    rivit.append(RaporttiRivi());

                    tilinLoppusaldo += saldo;
                }

            } else if ( tyyppi == 'M') {
                QVariantMap map = tieto.toMap();
                Euro loppusaldo = Euro::fromVariant(map.value("saldo"));

                if( loppusaldo || !piilotaNollat_) {
                    Euro ennen = Euro::fromVariant(map.value("ennen"));
                    tilinLoppusaldo += loppusaldo;

                    RaporttiRivi saldorivi;
                    saldorivi.lisaa(" ", 2);
                    saldorivi.lisaa( kaanna("Alkusaldo %1").arg( mista_.toString("dd.MM.yyyy")),2);
                    saldorivi.lisaa( ennen, true );
                    rivit.append(saldorivi);

                    // Muutokset
                    for( const QVariant& muutos : map.value("kausi").toList()) {
                        QVariantMap mmap = muutos.toMap();
                        RaporttiRivi rr;
                        lisaaTositeTunnus(&rr, mmap);
                        rr.lisaa( mmap.value("vientipvm").toDate());

                        QString kumppani = mmap.value("kumppani").toString();
                        QString selite = mmap.value("selite").toString();

                        if(kumppani.isEmpty() || selite == kumppani) {
                            rr.lisaa(selite, 2);
                        } else {
                            rr.lisaa( kumppani);
                            rr.lisaa( selite);
                        }

                        rr.lisaa( Euro::fromVariant(mmap.value("eur")));
                        rivit.append(rr);
                    }
                }

            } else if( tyyppi == 'E') {
                QVariantList erat = tieto.toList();

                for( const QVariant& era : qAsConst( erat )) {
                    QVariantMap emap = era.toMap();
                    RaporttiRivi rr;

                    if( emap.contains("id")) {
                        int eraid = emap.value("id").toInt();
                        lisaaTositeTunnus(&rr, emap);
                        rr.lisaa( emap.value("vientipvm").toDate());

                        QString kumppani = emap.value("kumppani").toString();
                        if( eraid % 10 == -4 ) // Kumppani-sarakkeen alkuun huoneiston tunnus, jos huoneistokohtainen lasku
                            kumppani = "(" + kp()->huoneistot()->tunnus(eraid / -10) + ") " + kumppani;

                        QString selite = emap.value("selite").toString();

                        if(kumppani.isEmpty() || selite == kumppani) {
                            rr.lisaa(selite, 2);
                        } else {
                            rr.lisaa( kumppani);
                            rr.lisaa( selite);
                        }

                    } else {
                        rr.lisaa("",2);
                        rr.lisaa(kaanna("Erittelemättömät"),2);
                    }
                    Euro euro = Euro::fromVariant(emap.value("eur"));
                    rr.lisaa(euro);
                    rivit.append(rr);
                    tilinLoppusaldo += euro;
                }
            }

            RaporttiRivi vikaRivi;
            vikaRivi.lisaa("", 2);
            vikaRivi.lisaa(kaanna("Tilin %1 loppusaldo").arg(tili->numero()),2);
            vikaRivi.lisaa( tilinLoppusaldo, true);
            vikaRivi.lihavoi();
            vikaRivi.viivaYlle();
            rivit.append(vikaRivi);
            yhteensa += tilinLoppusaldo;

            if( tilinLoppusaldo || !piilotaNollat_)
                rk.lisaaRivit(rivit);
        }        

        rk.lisaaRivi();
    }

    RaporttiRivi summaRivi;
    summaRivi.lisaa(kaanna("VASTATTAVAA YHTEENSÄ"),4);
    summaRivi.lisaa(yhteensa);
    rk.lisaaRivi(summaRivi);
    rk.lisaaTyhjaRivi();

    valmis();
}

void LaatijanTaseErittely::lisaaTositeTunnus(RaporttiRivi *rivi, const QVariantMap &map)
{
    QDate pvm = map.value("pvm").toDate();

    if( pvm < mista_ || pvm > mihin_) {
        // Tositetta ei ole tässä arkistossa
        rivi->lisaa( kp()->tositeTunnus(map.value("tunniste").toInt(),
                                        map.value("pvm").toDate(),
                                        map.value("sarja").toString()) );
    } else {
        rivi->lisaaTositeTunnus(map.value("pvm").toDate(), map.value("sarja").toString(), map.value("tunniste").toInt());
    }
}
