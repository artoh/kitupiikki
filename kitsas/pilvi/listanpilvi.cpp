#include "listanpilvi.h"

#include <QVariantMap>
#include <QImage>
#include <QByteArray>

ListanPilvi::ListanPilvi()
{

}

ListanPilvi::ListanPilvi(const QVariant &variant)
{
    const QVariantMap map = variant.toMap();

    id_ = map.value("id").toInt();
    nimi_ = map.value("name").toString();
    kokeilu_ = map.value("trial").toBool();
    ready_ = map.value("ready", true).toBool();

    badges_ = map.value("badges").toStringList();


    logo_ = QByteArray::fromBase64(map.value("logo").toByteArray());
}

void ListanPilvi::asetaBadget(const QStringList &lista)
{
    badges_ = lista;
}


