#include "laatijanalv.h"

#include "alv/alvlaskelma.h"

LaatijanAlv::LaatijanAlv(RaportinLaatija *laatija, const RaporttiValinnat &valinnat)
    : LaatijanRaportti(laatija, valinnat)
{

}

void LaatijanAlv::laadi()
{
    laskelma = new AlvLaskelma(this, kielikoodi());
    connect( laskelma, &AlvLaskelma::valmis, this, &LaatijanAlv::laadittu);
    laskelma->laske( valinnat().arvo(RaporttiValinnat::AlvAlkuPvm).toDate(),
                     valinnat().arvo(RaporttiValinnat::AlvLoppuPvm).toDate());
}

void LaatijanAlv::laadittu(RaportinKirjoittaja kirjoittaja)
{
    rk = kirjoittaja;
    if( laskelma )
        delete laskelma;
    laskelma = nullptr;
    valmis();
}
