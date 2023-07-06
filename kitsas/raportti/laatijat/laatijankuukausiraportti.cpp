#include "laatijankuukausiraportti.h"
#include "db/kirjanpito.h"

LaatijanKuukausiRaportti::LaatijanKuukausiRaportti(RaportinLaatija *laatija, const RaporttiValinnat &valinnat) :
    LaatijanRaportti(laatija, valinnat)
{

}

void LaatijanKuukausiRaportti::laadi()
{
    QVariantMap map;
    const QString tyyppi = valinnat().arvo(RaporttiValinnat::Tyyppi).toString();

    map.insert("tyyppi", tyyppi == "tulos" ? "tuloslaskelma" : "tase" );
    map.insert("kieli", valinnat().arvo(RaporttiValinnat::Kieli));
    map.insert("raportti", valinnat().arvo(RaporttiValinnat::RaportinMuoto));

    yhteensaSarake_ = valinnat().arvo(RaporttiValinnat::KuukaudetYhteensa).toBool() && tyyppi == "tulos";
    kieli_ = valinnat().arvo(RaporttiValinnat::Kieli).toString();

    const int kohdennuksella = valinnat().arvo(RaporttiValinnat::Kohdennuksella).toInt();
    if( kohdennuksella > -1) {
        map.insert("kohdennus", kohdennuksella);
        rk.asetaKausiteksti( kp()->kohdennukset()->kohdennus(kohdennuksella).nimi(kieli_) );
    }

    const QDate alkupvm = valinnat().arvo(RaporttiValinnat::AlkuPvm).toDate();
    const QDate loppupvm = valinnat().arvo(RaporttiValinnat::LoppuPvm).toDate();

    QDate pvm(alkupvm.year(), alkupvm.month(), 1);

    QVariantList kaudet;
    while( pvm < loppupvm) {
        QVariantMap kausi;
        kausi.insert("loppupvm", QDate(pvm.year(), pvm.month(), pvm.daysInMonth()).toString("yyyy-MM-dd"));
        kausi.insert("tyyppi", "toteutunut");
        if( tyyppi == "tulos") {
            kausi.insert("alkupvm", pvm.toString("yyyy-MM-dd"));
        }
        kaudet.append(kausi);
        pvm = pvm.addMonths(1);
    }
    map.insert("sarakkeet", kaudet);
    map.insert("erittely", valinnat().onko(RaporttiValinnat::TulostaErittely));

    KpKysely* kysely = kpk("/raportti", KpKysely::POST);
    connect( kysely, &KpKysely::vastaus, this, &LaatijanKuukausiRaportti::dataSaapuu);
    kysely->kysy(map);
}

QString LaatijanKuukausiRaportti::nimi() const
{
    return QString();
}

void LaatijanKuukausiRaportti::dataSaapuu(QVariant *variant)
{
    QVariantMap map = variant->toMap();
    RaporttiRivi otsake;

    rk.asetaOtsikko( map.value("otsikko").toString() );
    rk.lisaaVenyvaSarake(100, RaporttiRivi::KAIKKI, "1234 XXXXXXXXXXXX");
    otsake.lisaa("");

    QVariantList sarakkeet = map.value("otsakkeet").toList();
    for(const auto& sarake : sarakkeet) {
        rk.lisaaEurosarake();
        otsake.lisaa( sarake.toMap().value("loppupvm").toDate().toString("M/yyyy"), 1, true);
    }
    if( yhteensaSarake_ ) {
        rk.lisaaEurosarake();
        otsake.lisaa( tulkkaa("Yhteensä", kieli_), 1, true );
    }

    rk.lisaaOtsake(otsake);

    for(const auto& item : map.value("raportti").toList()) {
        QVariantMap row = item.toMap();
        RaporttiRivi rivi;
        const int sisennys = row.value("S").toInt();
        const QString muoto = row.value("M").toString();

        rivi.sisenna(sisennys);
        rivi.lisaa(row.value("teksti").toString());

        if( muoto.contains("M")) rivi.lihavoi(true);
        if( row.value("V").toInt()) rk.lisaaTyhjaRivi();

        Euro yhteensa = Euro::Zero;
        QVariantList summaLista = row.value("summat").toList();

        for(const QVariant& summa : summaLista) {
            rivi.lisaa(Euro(summa.toString()), true);
            if( yhteensaSarake_) yhteensa += Euro(summa.toString());
        }
        if( yhteensaSarake_ && !summaLista.isEmpty()) rivi.lisaa(yhteensa, true);
        rk.lisaaRivi(rivi);

        for(const QVariant& erittely : row.value("erittely").toList()) {
            QVariantMap eMap = erittely.toMap();
            RaporttiRivi eRivi;
            Euro eSumma = Euro::Zero;
            eRivi.sisenna(sisennys + 1);
            eRivi.lisaa(QString("%1 %2").arg(eMap.value("tili").toString(), eMap.value("nimi").toString()));
            for(const auto& summa: eMap.value("summat").toList()) {
                eRivi.lisaa(Euro(summa.toString()), true);
                if( yhteensaSarake_) eSumma += Euro(summa.toString());
            }
            if( yhteensaSarake_) eRivi.lisaa(eSumma, true);
            rk.lisaaRivi(eRivi);
        }
    }
    valmis();
}
