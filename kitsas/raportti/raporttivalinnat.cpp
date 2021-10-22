#include "raporttivalinnat.h"
#include "db/kirjanpito.h"
#include "db/tilikausi.h"

RaporttiValinnat::RaporttiValinnat()
{

}

void RaporttiValinnat::aseta(Valinta valinta, QVariant arvo)
{
    valinnat_.insert(valinta, arvo);
}

void RaporttiValinnat::asetaSarakkeet(QList<RaporttiValintaSarake> sarakkeet)
{
    sarakkeet_ = sarakkeet;
}

void RaporttiValinnat::nollaa()
{
    Tilikausi nykykausi = kp()->tilikausiPaivalle( kp()->paivamaara() );
    if( !nykykausi.alkaa().isValid())
        nykykausi = kp()->tilikaudet()->tilikausiIndeksilla( kp()->tilikaudet()->rowCount() - 1 );

    aseta( AlkuPvm, nykykausi.alkaa() );
    aseta( LoppuPvm, nykykausi.paattyy() );
    aseta( Kohdennuksella, -1);
}

RaporttiValintaSarake::RaporttiValintaSarake()
{

}

RaporttiValintaSarake::RaporttiValintaSarake(const QDate &loppuPvm) :
    loppuPvm_(loppuPvm)
{

}

RaporttiValintaSarake::RaporttiValintaSarake(const QDate &alkuPvm, const QDate &loppuPvm, SarakeTyyppi tyyppi) :
    alkuPvm_(alkuPvm), loppuPvm_(loppuPvm), tyyppi_(tyyppi)
{

}
