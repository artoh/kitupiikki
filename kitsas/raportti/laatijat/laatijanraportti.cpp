#include "laatijanraportti.h"
#include "../raportinlaatija.h"

#include "db/kirjanpito.h"

LaatijanRaportti::LaatijanRaportti(RaportinLaatija *laatija, const RaporttiValinnat &valinnat) :
    QObject(laatija), valinnat_(valinnat)
{
    kielikoodi_ = valinnat.arvo(RaporttiValinnat::Kieli).toString();
    rk.asetaKieli(kielikoodi_);
}

QString LaatijanRaportti::nimi() const
{
    return tulkkaa( valinnat().arvo(RaporttiValinnat::Tyyppi).toString(), kielikoodi() );
}

void LaatijanRaportti::valmis()
{
    RaportinLaatija* laatija = qobject_cast<RaportinLaatija*>(parent());
    laatija->valmis(this);
}

void LaatijanRaportti::tyhja()
{
    RaportinLaatija* laatija = qobject_cast<RaportinLaatija*>(parent());
    laatija->tyhja(this);
}

QString LaatijanRaportti::kaanna(const QString &teksti) const
{
    return tulkkaa(teksti, kielikoodi_);
}

QString LaatijanRaportti::alvTeksti(const QVariantMap &data)
{
    const int alvKoodi = data.value("alvkoodi").toInt() % 100;
    const int alvProsentti = qRound(data.value("alvprosentti").toDouble());

    switch(alvKoodi) {
    case AlvKoodi::EIALV: return QString();
    case AlvKoodi::ALV0: return "0%";
    case AlvKoodi::MYYNNIT_NETTO: case AlvKoodi::OSTOT_NETTO: return QString::number(alvProsentti) + "%";
    case AlvKoodi::MYYNNIT_BRUTTO: case AlvKoodi::OSTOT_BRUTTO: return "B " + QString::number(alvProsentti) + "%";
    case AlvKoodi::MYYNNIT_MARGINAALI: case AlvKoodi::OSTOT_MARGINAALI: return "M " + QString::number(alvProsentti) + "%";
    case AlvKoodi::YHTEISOHANKINNAT_PALVELUT: case AlvKoodi::YHTEISOHANKINNAT_TAVARAT: "EU " + QString::number(alvProsentti) + "%";
    case AlvKoodi::YHTEISOMYYNTI_PALVELUT: case AlvKoodi::YHTEISOMYYNTI_TAVARAT: return "EU";
    case AlvKoodi::RAKENNUSPALVELU_OSTO: return "R " + QString::number(alvProsentti) + "%";
    case AlvKoodi::RAKENNUSPALVELU_MYYNTI: return "R";
    case AlvKoodi::MAAHANTUONTI: case AlvKoodi::MAAHANTUONTI_PALVELUT: "X " + QString::number(alvProsentti) + "%";
    case AlvKoodi::MAKSUPERUSTEINEN_MYYNTI: case AlvKoodi::MAKSUPERUSTEINEN_OSTO: case AlvKoodi::ENNAKKOLASKU_MYYNTI: return "* " + QString::number(alvProsentti) + "%" ;
    }
    return "";
}

