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

LiiteCache::~LiiteCache()
{
    tyhjenna();
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

    // Koska tätä on vast'ikään haettu, sijoitetaan se listan kärkeen
    // jolloin sitä ei poisteta välittömästi
    if(poistoptr_ == liite && liite->seuraava()) {
        poistoptr_ = liite->seuraava();
    }
    liite->sijoitaKarkeen( uusin_ );
    uusin_ = liite;
    if( !poistoptr_)
        poistoptr_ = liite;


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
    liite->setTila( CacheLiite::HAETTU );

    koko_ += liite->size();

    // Jos välimuistin koko on ylitetty, poistetaan välimuistista riittävä määrä
    // liitteita, joita ei ole vähään aikaan haettu

    while( koko_ > rajaKoko_ && poistoptr_ && poistoptr_ != liite && poistoptr_->seuraava()) {
        if( poistoptr_->lukossa()) {
            // Jos poistopointterin alla oleva on lukittuna,
            // ei sitä poisteta vaan se siirretään kärkeen
            CacheLiite* seuraava = poistoptr_->seuraava();
            poistoptr_->sijoitaKarkeen(uusin_);
            uusin_ = poistoptr_;
            poistoptr_ = seuraava;
        } else {
            koko_ -= poistoptr_->size();
            poistoptr_->tyhjenna();
            poistoptr_ = poistoptr_->seuraava();
            qDebug() << " Poistettu liite, uusi koko " << koko_;
        }
    }


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
        if( iter.value()->lukossa()) {
            iter.value()->setTila(CacheLiite::KELVOTON);
        } else {
            delete iter.value();
        }
    }
    liitteet_.clear();

    uusin_ = nullptr;
    poistoptr_ = nullptr;
}

void LiiteCache::lisaaTallennettu(int liiteId, CacheLiite *liite)
{
    liitteet_.insert(liiteId, liite);

    liite->sijoitaKarkeen( uusin_ );
    uusin_ = liite;
    if( !poistoptr_)
        poistoptr_ = liite;
}
