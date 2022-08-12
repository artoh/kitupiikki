#include "pankki.h"

#include "db/kirjanpito.h"
#include <QNetworkRequest>
#include <QNetworkReply>


namespace Tilitieto {

Pankki::Pankki()
{

}

Pankki::Pankki(const QVariantMap& map)
{
    id_ = map.value("id").toInt();
    nimi_ = map.value("name").toString();
    bic_ = map.value("bic").toString();
    logoUrl_ = map.value("logo").toString();
}

QIcon Pankki::icon() const
{
    if( logo_.isNull()) {
        return QIcon(":/pic/tyhja.png");
    } else {
        return QIcon(QPixmap::fromImage(logo_.scaled(QSize(32,32))));
    }
}

void Pankki::setLogo(const QImage &logo)
{
    logo_ = logo;
}

} // namespace Tilitieto
