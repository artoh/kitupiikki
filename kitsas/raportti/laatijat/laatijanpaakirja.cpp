#include "laatijanpaakirja.h"
#include "db/kirjanpito.h"

LaatijanPaakirja::LaatijanPaakirja(RaportinLaatija *laatija, const RaporttiValinnat &valinnat) :
    LaatijanRaportti(laatija, valinnat)
{

}

void LaatijanPaakirja::laadi()
{
    const QDate mista = valinnat().arvo(RaporttiValinnat::AlkuPvm).toDate();
    const QDate mihin = valinnat().arvo(RaporttiValinnat::LoppuPvm).toDate();
    const int kohdennuksella = valinnat().arvo(RaporttiValinnat::Kohdennuksella).toInt();
    const int tililta = valinnat().arvo(RaporttiValinnat::Tililta).toInt();

    const Tilikausi alkukausi = kp()->tilikaudet()->tilikausiPaivalle(mista);
    const Tilikausi loppukausi = kp()->tilikaudet()->tilikausiPaivalle(mihin);
    samatilikausi_ = alkukausi.alkaa() == loppukausi.alkaa();

    KpKysely *saldokysely = kpk("/saldot");
    saldokysely->lisaaAttribuutti("pvm",mista);
    saldokysely->lisaaAttribuutti("alkusaldot");
    if( kohdennuksella > -1) {
        saldokysely->lisaaAttribuutti("kohdennus", kohdennuksella);
        saldokysely->lisaaAttribuutti("tuloslaskelma");
    }
    if( tililta )
        saldokysely->lisaaAttribuutti("tili", tililta);

    connect( saldokysely, &KpKysely::vastaus, this, &LaatijanPaakirja::saldotSaapuu);

    KpKysely *vientikysely = kpk("/viennit");
    vientikysely->lisaaAttribuutti("alkupvm", mista);
    vientikysely->lisaaAttribuutti("loppupvm", mihin);
    vientikysely->lisaaAttribuutti("jarjestys","tili");

    if( kohdennuksella > -1)
        vientikysely->lisaaAttribuutti("kohdennus", kohdennuksella);
    if( tililta )
        vientikysely->lisaaAttribuutti("tili", tililta);

    connect( vientikysely, &KpKysely::vastaus, this, &LaatijanPaakirja::viennitSaapuu);


    if( kohdennuksella > -1 )
        // Tulostetaan vain yhdestä kohdennuksesta
        rk.asetaOtsikko( QString(kaanna("PÄÄKIRJAN OTE") + "\n%1").arg( kp()->kohdennukset()->kohdennus(kohdennuksella).nimi()) ) ;
    else if( tililta)
        rk.asetaOtsikko( kaanna("PÄÄKIRJAN OTE"));
    else
        rk.asetaOtsikko( kaanna("PÄÄKIRJA"));

    rk.asetaKausiteksti(QString("%1 - %2").arg( mista.toString("dd.MM.yyyy") ,
                                             mihin.toString("dd.MM.yyyy") ) );

    rk.lisaaSarake("", RaporttiRivi::CSV);  // Tilin numero
    rk.lisaaSarake("", RaporttiRivi::CSV);  // Tilin nimi

    rk.lisaaPvmSarake();        // Pvm
    if( kp()->asetukset()->onko("erisarjaan") )
        rk.lisaaSarake("ABC1234/99 ");
    else
        rk.lisaaSarake("12345/99");

    if( valinnat().onko(RaporttiValinnat::TulostaKumppani) && valinnat().onko(RaporttiValinnat::TulostaKohdennus)) {
        rk.lisaaVenyvaSarake(100,RaporttiRivi::EICSV); // Kumppani + Selite
        rk.lisaaSarake("", RaporttiRivi::CSV);  // Kumppani
        rk.lisaaSarake("", RaporttiRivi::CSV);  // Selite
    } else if( valinnat().onko(RaporttiValinnat::TulostaKumppani)) {
        rk.lisaaVenyvaSarake(); // Kumppani
        rk.lisaaVenyvaSarake(); // Selite
    } else {
        rk.lisaaVenyvaSarake(); // Selite
    }


    if( valinnat().onko(RaporttiValinnat::TulostaKohdennus))
        rk.lisaaSarake("Kohdennusnimi"); // Kohdennus
    rk.lisaaEurosarake();   // Debet
    rk.lisaaEurosarake();   // Kredit
    rk.lisaaEurosarake();   // Saldo

    RaporttiRivi otsikko;
    otsikko.lisaa(kaanna("Tilin numero"));
    otsikko.lisaa(kaanna("Tilin nimi"));
    otsikko.lisaa(kaanna("Pvm"));
    otsikko.lisaa(kaanna("Tosite"));

    if( valinnat().onko(RaporttiValinnat::TulostaKumppani) && valinnat().onko(RaporttiValinnat::TulostaKohdennus)) {
        otsikko.lisaa( kaanna("Asiakas/Toimittaja") + ", " + kaanna("Selite") );
        otsikko.lisaa(kaanna("Asiakas/Toimittaja"));
        otsikko.lisaa(kaanna("Selite"));
    } else if( valinnat().onko(RaporttiValinnat::TulostaKumppani)) {
        otsikko.lisaa(kaanna("Asiakas/Toimittaja"));
        otsikko.lisaa(kaanna("Selite"));
    } else {
        otsikko.lisaa(kaanna("Selite"));
    }


    if( valinnat().onko(RaporttiValinnat::TulostaKohdennus) )
        otsikko.lisaa(kaanna("Kohdennus"));
    otsikko.lisaa(kaanna("Debet €"),1,true);
    otsikko.lisaa(kaanna("Kredit €"),1,true);
    otsikko.lisaa(kaanna("Saldo €"),1, true);
    rk.lisaaOtsake(otsikko);

    saldokysely->kysy();
    vientikysely->kysy();

}

void LaatijanPaakirja::saldotSaapuu(QVariant *data)
{
    QVariantMap saldot = data->toMap();
    QMapIterator<QString,QVariant> iter(saldot);
    while(iter.hasNext()) {
        iter.next();
        const QString tili = iter.key();
        saldot_.insert(tili, qRound64(iter.value().toDouble() * 100.0));
        if( !data_.contains(tili))
            data_.insert(tili, QList<QVariantMap>());
    }
    if( ++saapuneet_ > 1)
        kirjoitaDatasta();
}

void LaatijanPaakirja::viennitSaapuu(QVariant *data)
{
    const QVariantList viennit = data->toList();
    for(const auto& vienti : viennit) {
        QVariantMap map = vienti.toMap();
        const QString tili = map.value("tili").toString();
        data_[tili].append(map);
    }

    if( ++saapuneet_ > 1)
        kirjoitaDatasta();
}

void LaatijanPaakirja::kirjoitaDatasta()
{
    QMapIterator<QString, QList<QVariantMap>> iter(data_);

    qlonglong kaikkiDebet = 0;
    qlonglong kaikkiKredit = 0;

    const int otsikkosarakkeet =
            (valinnat().onko(RaporttiValinnat::TulostaKohdennus) && valinnat().onko(RaporttiValinnat::TulostaKumppani)) ? 8 :
            ((valinnat().onko(RaporttiValinnat::TulostaKohdennus) || valinnat().onko(RaporttiValinnat::TulostaKumppani)) ? 6 : 5 );

    const int summasarakkeet =
            (valinnat().onko(RaporttiValinnat::TulostaKohdennus) && valinnat().onko(RaporttiValinnat::TulostaKumppani)) ? 5 :
            ((valinnat().onko(RaporttiValinnat::TulostaKohdennus) || valinnat().onko(RaporttiValinnat::TulostaKumppani)) ? 4 : 4 );



    while( iter.hasNext()) {
        iter.next();


        Tili tili = kp()->tilit()->tiliNumerolla(iter.key().toInt());
        if( tili.onkoValidi())
        {

            RaporttiRivi rivi(RaporttiRivi::EICSV);
            rivi.lihavoi();
            rivi.lisaa("",2);
            rivi.lisaaLinkilla( RaporttiRiviSarake::TILI_LINKKI, tili.numero(),
                                tili.nimiNumero(kielikoodi()), otsikkosarakkeet);

            qlonglong saldo =  saldot_.value( QString::number(tili.numero() ));

            // #827 Ei näytä tyhjää otsikkoriviä esimerkiksi tilikauden tulokselle
            if( iter.value().isEmpty() && !saldo)
                continue;

            rivi.lisaa( saldo );
            rk.lisaaRivi(rivi);

            qlonglong debetSumma = 0l;
            qlonglong kreditSumma = 0l;

            for(const QVariantMap& vienti : iter.value()) {

                RaporttiRivi rr;

                QString tilinumero = vienti.value("tili").toString();
                rr.lisaa( tilinumero );
                rr.lisaa( kp()->tilit()->tili(tilinumero)->nimi(kielikoodi()));
                rr.lisaa( vienti.value("pvm").toDate() );

                QVariantMap tositeMap = vienti.value("tosite").toMap();

                rr.lisaaTositeTunnus( tositeMap.value("pvm").toDate(), tositeMap.value("sarja").toString(), tositeMap.value("tunniste").toInt(),
                                      samatilikausi_);

                const QString kumppani = vienti.value("kumppani").toMap().value("nimi").toString();
                const QString selite = vienti.value("selite").toString();
                const QString tselite = kumppani == selite ? "" : selite;

                if( valinnat().onko(RaporttiValinnat::TulostaKumppani) && valinnat().onko(RaporttiValinnat::TulostaKohdennus)) {
                    rr.lisaa ( kumppani + ( kumppani.isEmpty() || tselite.isEmpty() ? "" : "\n" ) + tselite );
                    rr.lisaa( kumppani );
                    rr.lisaa (selite );
                } else if( valinnat().onko(RaporttiValinnat::TulostaKumppani)) {
                    rr.lisaa( kumppani);
                    rr.lisaa( tselite );
                } else {
                    rr.lisaa( selite.isEmpty() ? kumppani : selite );
                }



                if( valinnat().onko(RaporttiValinnat::TulostaKohdennus))
                    rr.lisaa(kp()->kohdennukset()->kohdennus( vienti.value("kohdennus").toInt() ).nimi(kielikoodi()) );

                rr.lisaa(  vienti.value("debet").toDouble()  );
                rr.lisaa(  vienti.value("kredit").toDouble()  );

                debetSumma += qRound64( vienti.value("debet").toDouble() * 100 );
                kreditSumma += qRound64( vienti.value("kredit").toDouble() * 100 );

                if( tili.onko(TiliLaji::VASTAAVAA))
                {
                    saldo += qRound64( vienti.value("debet").toDouble() * 100 );
                    saldo -= qRound64( vienti.value("kredit").toDouble() * 100 );
                } else {
                    saldo -= qRound64( vienti.value("debet").toDouble() * 100 );
                    saldo += qRound64( vienti.value("kredit").toDouble() * 100 );
                }

                if( tili.onko(TiliLaji::TULOS) || valinnat().arvo(RaporttiValinnat::Kohdennuksella).toInt() < 0)
                    rr.lisaa( saldo,true);
                rk.lisaaRivi(rr);

            }
            if( (debetSumma || kreditSumma) && valinnat().onko(RaporttiValinnat::TulostaSummarivit)  ) {
                RaporttiRivi summa(RaporttiRivi::EICSV);
                summa.viivaYlle();
                summa.lihavoi();
                summa.lisaa("", summasarakkeet);

                if( valinnat().onko(RaporttiValinnat::TulostaKohdennus) )
                    summa.lisaa("");
                if( valinnat().onko(RaporttiValinnat::TulostaKumppani) )
                    summa.lisaa("");

                qlonglong muutos = tili.onko(TiliLaji::VASTAAVAA) ?
                        debetSumma - kreditSumma : kreditSumma - debetSumma;
                summa.lisaa(muutos,false, true);

                summa.lisaa(debetSumma);
                summa.lisaa(kreditSumma);

                if( tili.onko(TiliLaji::TULOS) || valinnat().arvo(RaporttiValinnat::Kohdennuksella).toInt() < 0)
                    summa.lisaa(saldo);

                rk.lisaaRivi(summa);

                kaikkiDebet += debetSumma;
                kaikkiKredit += kreditSumma;
            }
            rk.lisaaRivi();

        }

    }
    if( valinnat().onko(RaporttiValinnat::TulostaSummarivit) ) {
        RaporttiRivi summa(RaporttiRivi::EICSV);
        summa.viivaYlle();
        summa.lihavoi();
        summa.lisaa("",2);
        summa.lisaa(kaanna("Yhteensä"),
                    summasarakkeet + (valinnat().onko(RaporttiValinnat::TulostaKohdennus) && valinnat().onko(RaporttiValinnat::TulostaKumppani) ? 1 : 0)) ;


        summa.lisaa(kaikkiDebet);
        summa.lisaa(kaikkiKredit);
        summa.lisaa("");

        rk.lisaaRivi(summa);
    }


    valmis();
}
