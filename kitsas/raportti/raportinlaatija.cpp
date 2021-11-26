#include "raportinlaatija.h"
#include "laatijat/laatijanraportti.h"
#include "laatijat/laatijanpaivakirja.h"
#include "laatijat/laatijanpaakirja.h"
#include "laatijat/laatijantositeluettelo.h"
#include "laatijat/laatijantaseerittely.h"
#include "laatijat/laatijanmyynti.h"
#include "laatijat/laatijanlaskut.h"
#include "laatijat/laatijantilikartta.h"
#include "laatijat/laatijantasetulos.h"
#include "laatijat/laatijanalv.h"

RaportinLaatija::RaportinLaatija(QObject *parent) : QObject(parent)
{

}

void RaportinLaatija::laadi(const RaporttiValinnat &valinnat)
{
    LaatijanRaportti* raportti = nullptr;
    const QString raporttiTyyppi = valinnat.arvo(RaporttiValinnat::Tyyppi).toString();

    if(raporttiTyyppi == "paivakirja")
        raportti = new LaatijanPaivakirja(this, valinnat);
    else if(raporttiTyyppi == "paakirja")
        raportti = new LaatijanPaakirja(this, valinnat);
    else if(raporttiTyyppi == "tositeluettelo")
        raportti = new LaatijanTositeLuettelo(this, valinnat);
    else if(raporttiTyyppi == "taseerittely")
        raportti = new LaatijanTaseErittely(this, valinnat);
    else if(raporttiTyyppi == "myynti")
        raportti = new LaatijanMyynti(this, valinnat);
    else if(raporttiTyyppi == "laskut")
        raportti = new LaatijanLaskut(this, valinnat);
    else if(raporttiTyyppi == "tilikartta")
        raportti = new LaatijanTilikartta(this, valinnat);
    else if(raporttiTyyppi == "alv")
        raportti = new LaatijanAlv(this, valinnat);
    else
        raportti = new LaatijanTaseTulos(this, valinnat);

    raportti->laadi();

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
