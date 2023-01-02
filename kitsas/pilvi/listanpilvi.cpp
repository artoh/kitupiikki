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

    const QByteArray ba = QByteArray::fromBase64(map.value("logo").toByteArray());
    if( ba.isEmpty() ) {
        logo_ = QPixmap(":/pic/tyhja.png").scaled(32,32);
    } else {
        logo_ = QPixmap::fromImage( QImage::fromData(ba).scaled(32,32,Qt::KeepAspectRatio, Qt::SmoothTransformation) );
    }
}
