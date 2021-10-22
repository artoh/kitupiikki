#include "raporttivalinnat.h"

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
