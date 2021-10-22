#include "raportinlaatija.h"
#include "laatijat/laatijanraportti.h"
#include "laatijat/laatijanpaivakirja.h"

RaportinLaatija::RaportinLaatija(QObject *parent) : QObject(parent)
{

}

void RaportinLaatija::laadi(const RaporttiValinnat &valinnat)
{
    LaatijanRaportti* raportti = nullptr;
    const QString raporttiTyyppi = valinnat.arvo(RaporttiValinnat::Tyyppi).toString();

    if(raporttiTyyppi == "paivakirja")
        raportti = new LaatijanPaivakirja(this, valinnat);


    if( raportti ) {
        raportti->laadi();
    }
}

void RaportinLaatija::valmis(LaatijanRaportti *raportti)
{
    emit raporttiValmis(raportti->raportinKirjoittaja(), raportti->valinnat());
    raportti->deleteLater();
}

void RaportinLaatija::tyhja(LaatijanRaportti *raportti)
{
    emit tyhjaRaportti(raportti->valinnat());
    raportti->deleteLater();
}
