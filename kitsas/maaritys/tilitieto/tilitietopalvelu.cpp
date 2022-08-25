#include "tilitietopalvelu.h"

#include "pankitmodel.h"
#include "db/kirjanpito.h"
#include "pilvi/pilvimodel.h"
#include "pilvi/pilvikysely.h"
#include "kieli/kielet.h"
#include "pankkilokimodel.h"

namespace Tilitieto {

TilitietoPalvelu::TilitietoPalvelu(QObject *parent) :
    QObject(parent),
    pankit_(new PankitModel(this)),
    loki_(new PankkiLokiModel(this))
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
        if( yhteys.pankki().bic() == bic) {
            return true;
        }
    }
    return false;
}

void TilitietoPalvelu::lataa()
{       
    if( kp()->pilvi()->kbcOsoite().isEmpty())
        return;

    pankit_->haePankit();        

    if( ! qobject_cast<PilviModel*>( kp()->yhteysModel() ) ) {
        return;
    }

    const QString url = kp()->pilvi()->kbcOsoite() + "/api";
    PilviKysely *pk = new PilviKysely( kp()->pilvi(), KpKysely::GET, url);
    connect( pk, &PilviKysely::vastaus, this, &TilitietoPalvelu::lataaMap);
    pk->kysy();
}

void TilitietoPalvelu::haeTapahtumat(const Iban &iban, const QDate &mista, const QDate &mihin)
{
    QVariantMap map;
    map.insert("iban", iban.valeitta());
    map.insert("dateFrom", mista.toString("yyyy-MM-dd"));
    map.insert("dateTo", mihin.toString("yyyy-MM-dd"));

    const QString url = kp()->pilvi()->kbcOsoite() + "/fetch";
    PilviKysely *pk = new PilviKysely( kp()->pilvi(), KpKysely::POST, url );
    connect(pk, &PilviKysely::vastaus, this, &TilitietoPalvelu::tapahtumatHaettu);

    kp()->odotusKursori(true);

    pk->kysy(map);

}

QDateTime TilitietoPalvelu::seuraavaUusinta()
{
    QDateTime seuraava;
    for(int i=0; i < yhteyksia(); i++) {
        QDateTime voimassa = yhteys(i).voimassa();
        if( voimassa.isValid() &&
           ( seuraava.isNull() || voimassa < seuraava))  {
            seuraava = voimassa;
        }
    }
    return seuraava;
}

void TilitietoPalvelu::tapahtumatHaettu()
{
    kp()->odotusKursori(false);
    lataa();
}

void TilitietoPalvelu::lataaMap(const QVariant *data)
{
    QVariantMap map = data ? data->toMap() : QVariantMap();

    price_ = Euro::fromString(map.value("price").toString());

    trialPeriod_ = map.value("trialperiod").toDate();
    trialDays_ = map.value("trialdays").toInt();

    loki_->lataa( map.value("log").toList() );

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
