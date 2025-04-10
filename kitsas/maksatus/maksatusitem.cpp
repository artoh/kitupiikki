#include "maksatusitem.h"

MaksatusItem::MaksatusItem() {}

MaksatusItem::MaksatusItem(const QVariant &variant)
{
    lataa(variant);
}

void MaksatusItem::lataa(const QVariant &variant)
{
    QVariantMap map = variant.toMap();

    id_ = map.value("id").toString();
    tila_ = strToTila( map.value("tila").toString());
    euro_ = Euro::fromString( map.value("euro").toString() );
    pvm_ = map.value("pvm").toDate();
    viite_ = map.value("viite").toString();
    viesti_ = map.value("viesti").toString();

}


QString MaksatusItem::viiteTaiViesti() const
{
    if( viite_.isEmpty())
        return viesti_;
    else
        return viite_;
}

MaksatusItem::MaksatusTila MaksatusItem::strToTila(const QString &tilaStr)
{
    if( tilaStr == "PENDING")
        return MaksatusTila::PENDING;
    else if( tilaStr == "PENDING_CONFIRMATION")
        return MaksatusTila::PENDING_CONFIRMATION;
    else if( tilaStr == "INPROGRESS")
        return MaksatusTila::INPROGRESS;
    else if( tilaStr == "PAID")
        return MaksatusTila::PAID;
    else if( tilaStr == "REJECTED")
        return MaksatusTila::REJECTED;
    else if( tilaStr == "SCHEDULED")
        return MaksatusTila::SCHEDULED;
    else if( tilaStr == "ERROR")
        return MaksatusTila::ERROR;

    return MaksatusTila::UNKNOWN;
}
