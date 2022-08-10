#include "tilitietopalvelu.h"

#include "pankitmodel.h"
#include "db/kirjanpito.h"
#include "pilvi/pilvimodel.h"
#include "pilvi/pilvikysely.h"
#include "kieli/kielet.h"

namespace Tilitieto {

TilitietoPalvelu::TilitietoPalvelu(QObject *parent) :
    QObject(parent),
    pankit_(new PankitModel(this))
{

}


PankitModel *TilitietoPalvelu::pankit()
{
    return pankit_;
}

void TilitietoPalvelu::lisaaValtuutus(int pankkiId)
{
    const QString url = kp()->pilvi()->kbcOsoite() + QString("/api/%1").arg(pankkiId);
    PilviKysely *pk = new PilviKysely( kp()->pilvi(), KpKysely::POST, url);
    pk->lisaaAttribuutti("lang", Kielet::instanssi()->uiKieli());
    connect( pk, &PilviKysely::vastaus, this, &TilitietoPalvelu::linkkiSaapuu);
    pk->kysy();
}


void TilitietoPalvelu::poistaValtuutus(int pankkiId)
{
    const QString url = kp()->pilvi()->kbcOsoite() + QString("/api/%1").arg(pankkiId);
    PilviKysely *pk = new PilviKysely( kp()->pilvi(), KpKysely::DELETE, url);
    connect( pk, &PilviKysely::vastaus, this, &TilitietoPalvelu::lataa);
    pk->kysy();
}

int TilitietoPalvelu::yhteyksia() const
{
    return yhteydet_.count();
}

Yhteys TilitietoPalvelu::yhteys(int indeksi)
{
    return yhteydet_.at(indeksi);
}

bool TilitietoPalvelu::onkoValtuutettu(const QString &bic)
{
    for(const auto& yhteys : yhteydet_) {
        if( yhteys.pankki()->bic() == bic) {
            return true;
        }
    }
    return false;
}

void TilitietoPalvelu::lataa()
{
    const QString url = kp()->pilvi()->kbcOsoite() + "/api";
    PilviKysely *pk = new PilviKysely( kp()->pilvi(), KpKysely::GET, url);
    connect( pk, &PilviKysely::vastaus, this, &TilitietoPalvelu::lataaMap);
    pk->kysy();
}

void TilitietoPalvelu::lataaMap(const QVariant *data)
{
    QVariantMap map = data->toMap();

    price_ = Euro::fromString(map.value("price").toString());
    trialPeriod_ = map.value("trialdays").toInt();

    yhteydet_.clear();
    QVariantList list = map.value("connections").toList();
    for(const auto& item : list) {
        QVariantMap map = item.toMap();
        yhteydet_.append( Yhteys(map, pankit_) );
    }
    emit ladattu();
}

void TilitietoPalvelu::linkkiSaapuu(const QVariant *data)
{
    const QVariantMap map = data->toMap();
    const QString linkki = map.value("link").toString();
    int pankkiId = map.value("institution").toInt();
    emit vahvistaLinkilla(linkki, pankkiId);
}


} // namespace Tilitieto
