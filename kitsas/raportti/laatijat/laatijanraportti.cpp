#include "laatijanraportti.h"
#include "../raportinlaatija.h"

#include "db/kirjanpito.h"

LaatijanRaportti::LaatijanRaportti(RaportinLaatija *laatija, const RaporttiValinnat &valinnat) :
    QObject(laatija), valinnat_(valinnat)
{
    kielikoodi_ = valinnat.arvo(RaporttiValinnat::Kieli).toString();
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
