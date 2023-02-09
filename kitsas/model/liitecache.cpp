#include "liitecache.h"

#include "cacheliite.h"
#include <QVariant>

#include "db/kitsasinterface.h"
#include "db/yhteysmodel.h"

LiiteCache::LiiteCache(QObject *parent, KitsasInterface *kitsas)
    : QObject{parent},
      kitsas_{kitsas}
{

}

CacheLiite *LiiteCache::liite(int liiteId)
{
    return haku(liiteId, false);
}


void LiiteCache::ennakkoHaku(int liiteId)
{
    haku(liiteId, true);
}

void LiiteCache::tositteenLiitteidenEnnakkoHaku(int tositeId)
{
    if( !kitsas_->yhteysModel()) return;
    KpKysely* kysely = kitsas_->yhteysModel()->kysely(QString("/tositteet/%1").arg(tositeId));
    if( kysely ) {
        connect(kysely, &KpKysely::vastaus, this, &LiiteCache::tositeSaapuuu);
        kysely->kysy();
    }
}

CacheLiite *LiiteCache::haku(int liiteId, bool ennakkohaku)
{
    CacheLiite* liite = liitteet_.value(liiteId, nullptr);
    if( !liite) {
        liite = new CacheLiite(CacheLiite::ALUSTAMATON);
        liitteet_.insert(liiteId, liite);
    }
    if( liite->tila() == CacheLiite::ALUSTAMATON || liite->tila() == CacheLiite::TYHJENNETTY) {
        liite->setTila(ennakkohaku ? CacheLiite::ENNAKKOHAETAAN : CacheLiite::HAETAAN);
        if(!kaynnistaHaku(liiteId)) {
            liite->setTila(CacheLiite::ALUSTAMATON);
            return liite;
        }
    } else if( liite->tila() == CacheLiite::ENNAKKOHAETAAN && !ennakkohaku) {
        liite->setTila(CacheLiite::HAETAAN);
    }
    return liite;
}


bool LiiteCache::kaynnistaHaku(int liiteId)
{
    if( !kitsas_->yhteysModel()) return false;
    KpKysely* kysely = kitsas_->yhteysModel()->kysely(QString("/liitteet/%1").arg(liiteId));
    if( kysely ) {
        connect(kysely, &KpKysely::vastaus, this, [this, liiteId] (QVariant* data) { this->liiteSaapuu(liiteId, data); });
        kysely->kysy();
        return true;
    }
    return false;
}

void LiiteCache::liiteSaapuu(int liiteId, QVariant *data)
{
    CacheLiite* liite = liitteet_.value(liiteId, nullptr);
    if( !liite) {
        liite = new CacheLiite();
        liitteet_.insert(liiteId, liite);
    }
    bool ilmoita = liite->tila() == CacheLiite::LiiteTila::HAETAAN;

    liite->setData( data->toByteArray() );
    if( ilmoita ) {
        emit liiteHaettu(liiteId);
    }

}

void LiiteCache::tositeSaapuuu(QVariant *data)
{
    QVariantMap map = data->toMap();
    for(const auto& item : map.value("liitteet").toList()) {
        QVariantMap liiteMap = item.toMap();
        ennakkoHaku( liiteMap.value("id").toInt() );
    }
}

void LiiteCache::tyhjenna()
{
    QHashIterator<int, CacheLiite*> iter(liitteet_);
    while(iter.hasNext()) {
        iter.next();
        delete iter.value();
    }
    liitteet_.clear();
}
