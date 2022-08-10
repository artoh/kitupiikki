#include "pankki.h"

#include "db/kirjanpito.h"
#include <QNetworkRequest>
#include <QNetworkReply>


namespace Tilitieto {

Pankki::Pankki(QObject *parent) :
    QObject(parent)
{

}

Pankki::Pankki(QVariantMap map, QObject* parent) :
    QObject(parent)
{
    id_ = map.value("id").toInt();
    nimi_ = map.value("name").toString();
    bic_ = map.value("bic").toString();

    QNetworkReply* reply = kp()->networkManager()->get( QNetworkRequest(map.value("logo").toString()) );
    connect( reply, &QNetworkReply::finished, this, &Pankki::logoSaapuu);

}

QIcon Pankki::icon() const
{
    return QIcon(QPixmap::fromImage(logo_.scaled(QSize(32,32))));
}

void Pankki::logoSaapuu()
{
    QNetworkReply *reply = qobject_cast<QNetworkReply*>(sender());
    QByteArray ba = reply->readAll();
    logo_ = QImage::fromData(ba);

}

} // namespace Tilitieto
