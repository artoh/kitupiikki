#include "raporttivalinnat.h"
#include "db/kirjanpito.h"
#include "db/tilikausi.h"
#include "kieli/kielet.h"

RaporttiValinnat::RaporttiValinnat()
{
    aseta(Kohdennuksella, -1);
    aseta(Kieli, Kielet::instanssi()->nykyinen());
}

RaporttiValinnat::RaporttiValinnat(const QString &tyyppi)
{
    aseta(Kohdennuksella, -1);
    aseta(Kieli, Kielet::instanssi()->nykyinen());
    if( tyyppi.contains("/")) {
        const int kautta = tyyppi.indexOf('/');
        aseta(Tyyppi, tyyppi.left(kautta));
        aseta(RaportinMuoto, tyyppi);
    } else {
        aseta(Tyyppi, tyyppi);
    }
}

RaporttiValinnat::RaporttiValinnat(const RaporttiValinnat &toinen)
{
    valinnat_ = toinen.valinnat_;
    sarakkeet_ = toinen.sarakkeet_;
}

void RaporttiValinnat::aseta(Valinta valinta, QVariant arvo)
{
    valinnat_.insert(valinta, arvo);
}

void RaporttiValinnat::tyhjennaSarakkeet()
{
    sarakkeet_.clear();
}

void RaporttiValinnat::lisaaSarake(const RaporttiValintaSarake &sarake)
{
    sarakkeet_.append(sarake);
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

    QDate alvPvm = kp()->paivamaara().addMonths(-1);
    QDate alvAlku(alvPvm.year(), alvPvm.month(), 1);
    aseta( AlvAlkuPvm, alvAlku);
    aseta( AlvLoppuPvm, alvAlku.addMonths(1).addDays(-1));
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
