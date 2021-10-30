#include "laatijanmyynti.h"
#include "db/kirjanpito.h"

LaatijanMyynti::LaatijanMyynti(RaportinLaatija *laatija, const RaporttiValinnat &valinnat) :
    LaatijanRaportti(laatija, valinnat)
{

}

void LaatijanMyynti::laadi()
{
    const QDate mista = valinnat().arvo(RaporttiValinnat::AlkuPvm).toDate();
    const QDate mihin = valinnat().arvo(RaporttiValinnat::LoppuPvm).toDate();

    rk.asetaOtsikko(kaanna("MYYNTI"));
    rk.asetaKausiteksti(QString("%1 - %2")
                        .arg(mista.toString("dd.MM.yyyy"), mihin.toString("dd.MM.yyyy")));

    rk.lisaaVenyvaSarake();
    rk.lisaaEurosarake();
    rk.lisaaEurosarake();
    rk.lisaaEurosarake();
    rk.lisaaEurosarake();
    rk.lisaaEurosarake();

    RaporttiRivi otsikko;
    otsikko.lisaa(kaanna("Tuote"));
    otsikko.lisaa(kaanna("myynti kpl"),1,true);
    otsikko.lisaa(kaanna("keskim. á netto"),1,true);
    otsikko.lisaa(kaanna("keskim. á brutto"),1,true);
    otsikko.lisaa(kaanna("netto yht"),1,true);
    otsikko.lisaa(kaanna("brutto yht"),1,true);
    rk.lisaaOtsake(otsikko);

    KpKysely *kysely = kpk("/tuotteet/myynti");
    kysely->lisaaAttribuutti("alkupvm", mista);
    kysely->lisaaAttribuutti("loppupvm", mihin);

    connect( kysely, &KpKysely::vastaus, this, &LaatijanMyynti::dataSaapuu);
    kysely->kysy();
}

void LaatijanMyynti::dataSaapuu(QVariant *data)
{
    QVariantList lista = data->toList();
    QVariantMap epatuotteet;
    double yhteensa = 0;
    double bruttoyhteensa = 0;

    for(const auto& item : qAsConst( lista )) {
        const QVariantMap& map = item.toMap();
        if( !map.contains("tuote")  || map.value("tuote").toInt() == 0) {
            epatuotteet = map;
            continue;
        }
        RaporttiRivi rivi;
        rivi.lisaa( map.value("nimike").toString() );
        double kpl = map.value("kpl").toDouble();
        rivi.lisaa( kpl );
        double myynti = map.value("myynti").toDouble();
        double brutto = map.value("brutto").toDouble();
        if( qAbs(kpl) > 1e-5) {
            rivi.lisaa( myynti / kpl );
            rivi.lisaa( brutto / kpl );
        } else {
            rivi.lisaa("");
            rivi.lisaa("");
        }
        rivi.lisaa(myynti);
        rivi.lisaa(brutto);
        yhteensa += myynti;
        bruttoyhteensa += brutto;

        rk.lisaaRivi(rivi);
    }

    if(!epatuotteet.isEmpty()) {
        RaporttiRivi lisarivi(RaporttiRivi::EICSV);
        lisarivi.lisaa(kaanna("Muu myynti"),4);
        lisarivi.lisaa(epatuotteet.value("myynti").toDouble());
        lisarivi.lisaa(epatuotteet.value("brutto").toDouble());
        yhteensa += epatuotteet.value("myynti").toDouble();
        bruttoyhteensa += epatuotteet.value("brutto").toDouble();
        rk.lisaaRivi(lisarivi);
    }

    RaporttiRivi summarivi(RaporttiRivi::EICSV);
    summarivi.lisaa(kaanna("Yhteensä"),4);
    summarivi.lisaa(yhteensa);
    summarivi.lisaa(bruttoyhteensa);
    summarivi.viivaYlle();
    rk.lisaaRivi(summarivi);


    valmis();
}
