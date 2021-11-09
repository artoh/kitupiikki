#include "laatijanlaskut.h"

#include "db/kirjanpito.h"
#include "db/tositetyyppimodel.h"

LaatijanLaskut::LaatijanLaskut(RaportinLaatija *laatija, const RaporttiValinnat &valinnat) :
    LaatijanRaportti(laatija, valinnat)
{

}

void LaatijanLaskut::laadi()
{
    const QString rajaustyyppi = valinnat().arvo(RaporttiValinnat::LaskuRajausTyyppi).toString();
    const QDate mista = valinnat().arvo(RaporttiValinnat::AlkuPvm).toDate();
    const QDate mihin = valinnat().arvo(RaporttiValinnat::LoppuPvm).toDate();

    saldopvm_ = valinnat().arvo(RaporttiValinnat::SaldoPvm).toDate();
    lajittelu_ = valinnat().arvo(RaporttiValinnat::LaskunLajittelu).toString();
    myyntilaskut_ = valinnat().arvo(RaporttiValinnat::LaskuTyyppi).toString() == "myynti";

    KpKysely *kysely = myyntilaskut_  ? kpk("/myyntilaskut") : kpk("/ostolaskut");
    if( rajaustyyppi == "laskupvm" ) {
        kysely->lisaaAttribuutti("alkupvm", mista);
        kysely->lisaaAttribuutti("loppupvm", mihin);
    } else if( rajaustyyppi == "erapvm" ) {
        kysely->lisaaAttribuutti("eraalkupvm", mista);
        kysely->lisaaAttribuutti("eraloppupvm", mihin);
    }
    if( valinnat().onko(RaporttiValinnat::VainAvoimet))
        kysely->lisaaAttribuutti("avoin", QString());
    if( valinnat().onko(RaporttiValinnat::VainKitsaalla) )
        kysely->lisaaAttribuutti("kitsaslaskut", QString());

    kysely->lisaaAttribuutti("saldopvm", saldopvm_);

    connect( kysely, &KpKysely::vastaus, this, &LaatijanLaskut::dataSaapuu);
    kysely->kysy();
}

void LaatijanLaskut::dataSaapuu(QVariant *data)
{
    QVariantList list = data->toList();

    if( lajittelu_ != "pvm" )
        std::sort(list.begin(), list.end(), [this] (const QVariant& eka, const QVariant& toka)
            { return this->lajitteluVertailu(eka, toka);});


    if( valinnat().onko(RaporttiValinnat::VainAvoimet))
        rk.asetaOtsikko( myyntilaskut_ ? kaanna("AVOIMET MYYNTILASKUT") : kaanna("AVOIMET OSTOLASKUT") );
    else
        rk.asetaOtsikko( myyntilaskut_ ? kaanna("MYYNTILASKUT") : kaanna("OSTOLASKUT") );

    rk.asetaKausiteksti( saldopvm_.toString("dd.MM.yyyy"));

    rk.lisaaSarake( valinnat().onko(RaporttiValinnat::NaytaViitteet) ? "XXXXXXXXXXXXXXX" : "XXXXXX");  // Numero / Viite
    rk.lisaaPvmSarake();                // Laskupvm
    rk.lisaaPvmSarake();                // Eräpäivä
    rk.lisaaEurosarake();               // Summa
    rk.lisaaEurosarake();               // Avoinna
    rk.lisaaVenyvaSarake();             // Asiakas
    rk.lisaaVenyvaSarake();             // Selite

    RaporttiRivi otsikko;
    otsikko.lisaa( valinnat().onko(RaporttiValinnat::NaytaViitteet) ? kaanna("Viite") : kaanna("Numero") );
    otsikko.lisaa( kaanna("Laskupvm"));
    otsikko.lisaa( kaanna("Eräpvm"));
    otsikko.lisaa( kaanna("Summa"));
    otsikko.lisaa( kaanna("Maksamatta"));
    otsikko.lisaa( myyntilaskut_ ? kaanna("Asiakas") : kaanna("Toimittaja"));
    otsikko.lisaa(kaanna("Selite"));
    rk.lisaaOtsake(otsikko);

    qlonglong kokosumma = 0;
    qlonglong avoinsumma = 0;

    for(const auto& item : qAsConst( list )) {
        QVariantMap map = item.toMap();

        RaporttiRivi rivi;

        rivi.lisaa( valinnat().onko(RaporttiValinnat::NaytaViitteet) ? map.value("viite").toString() : map.value("numero").toString());
        rivi.lisaa( map.value("pvm").toDate());
        rivi.lisaa( map.value("erapvm").toDate());

        qlonglong summa = qRound64( map.value("summa").toDouble() * 100.0);
        rivi.lisaa( summa );
        kokosumma += summa;

        qlonglong avoin = qRound64( map.value("avoin").toDouble() * 100.0);
        rivi.lisaa( avoin );

        int tyyppi = map.value("tyyppi").toInt();

        // Hyvityslaskuja ja maksumuistutuksia ei lisätä avointen summaan, koska ovat jo laskun osalta
        if( tyyppi != TositeTyyppi::HYVITYSLASKU && tyyppi != TositeTyyppi::MAKSUMUISTUTUS)
            avoinsumma += avoin;

        QString asiakastoimittaja = myyntilaskut_ ? map.value("asiakas").toString() : map.value("toimittaja").toString();
        QString selite = map.value("selite").toString();

        rivi.lisaa(asiakastoimittaja);
        rivi.lisaa(asiakastoimittaja==selite ? "" : selite);

        rk.lisaaRivi(rivi);
    }

    if( valinnat().onko(RaporttiValinnat::TulostaSummarivit)) {
        RaporttiRivi summarivi(RaporttiRivi::EICSV);
        summarivi.lisaa(kaanna("Yhteensä"), 3);
        summarivi.lisaa(kokosumma);
        summarivi.lisaa(avoinsumma);
        summarivi.lisaa("",2);
        summarivi.viivaYlle();
        rk.lisaaRivi(summarivi);
    }

    valmis();
}

bool LaatijanLaskut::lajitteluVertailu(const QVariant &eka, const QVariant &toka)
{
    const QVariantMap& ekaMap = eka.toMap();
    const QVariantMap& tokaMap = toka.toMap();

    if( lajittelu_ == "numero" )
        return ekaMap.value("numero").toInt() < tokaMap.value("numero").toInt();
    else if( lajittelu_ == "viite" )
        return ekaMap.value("viite").toString() < tokaMap.value("viite").toString();
    else if( lajittelu_ == "erapvm")
        return ekaMap.value("erapvm").toDate() < tokaMap.value("erapvm").toDate();
    else if( lajittelu_ == "asiakas" && myyntilaskut_)
        return ekaMap.value("asiakas").toString() < tokaMap.value("asiakas").toString();
    else if( lajittelu_ == "asiakas" && !myyntilaskut_)
        return ekaMap.value("toimittaja").toString() < tokaMap.value("toimittaja").toString();
    else if( lajittelu_ == "summa")
        return ekaMap.value("summa").toDouble() > tokaMap.value("summa").toDouble();
    else
        return ekaMap.value("pvm").toDate() < tokaMap.value("pvm").toDate();
}
