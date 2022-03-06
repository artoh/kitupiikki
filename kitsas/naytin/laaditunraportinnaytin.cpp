#include "laaditunraportinnaytin.h"
#include "db/kirjanpito.h"

namespace Naytin {


LaaditunRaportinNaytin::LaaditunRaportinNaytin(QWidget *parent, RaporttiValinnat valinnat) :
    RaporttiNaytin( RaportinKirjoittaja(), parent ), laatija_(this)
{
    connect( &laatija_, &RaportinLaatija::raporttiValmis, this, &LaaditunRaportinNaytin::raporttiSaapuu );
    connect(&laatija_, &RaportinLaatija::tyhjaRaportti, this, &LaaditunRaportinNaytin::tyhja);
    paivitaRaportti(valinnat);
}

void LaaditunRaportinNaytin::paivitaRaportti()
{
    kp()->odotusKursori(true);
    laatija_.laadi(valinnat_);
}

void LaaditunRaportinNaytin::paivitaRaportti(const RaporttiValinnat &valinnat)
{
    valinnat_ = valinnat;
    paivitaRaportti();
}

void LaaditunRaportinNaytin::virkista()
{
    paivitaRaportti();
}

void LaaditunRaportinNaytin::raporttiSaapuu(const RaportinKirjoittaja &kirjoittaja, const RaporttiValinnat &valinnat)
{
    raportti_ = kirjoittaja;
    valinnat_ = valinnat;
    otsikko_ = kirjoittaja.otsikko() + " " + kirjoittaja.kausiteksti();
    emit otsikkoVaihtui(otsikko_);
    kp()->odotusKursori(false);
    paivita();
}

void LaaditunRaportinNaytin::tyhja()
{
    emit otsikkoVaihtui(tr("Tyhjä raportti"));
    emit eiSisaltoa();
    kp()->odotusKursori(false);
}



}
