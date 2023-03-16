#include "liitecache.h"

#include "cacheliite.h"
#include <QVariant>
#include <QJsonDocument>

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
    karkeen(liite);


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
        connect(kysely, &KpKysely::virhe, this, [this, liiteId] (int virhe)  { emit this->hakuVirhe(virhe, liiteId); });
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
        // Uusi liite kärkeen
        karkeen( liite );
    }    
    bool ilmoita = liite->tila() == CacheLiite::LiiteTila::HAETAAN;

    QJsonDocument doc = QJsonDocument::fromVariant( *data );

    liite->setData( doc.isEmpty() ? data->toByteArray() : doc.toJson());
    liite->setTila( CacheLiite::HAETTU );

    koko_ += liite->size();

    // Jos välimuistin koko on ylitetty, poistetaan välimuistista riittävä määrä
    // liitteita, joita ei ole vähään aikaan haettu


    while( koko_ > rajaKoko_ && vanhin_ != liite && vanhin_->seuraava()) {
        if( vanhin_->lukossa()) {
            karkeen( vanhin_ );
            qDebug() << " Kärkeen ";
            break;
        } else {
            // Poistetaan vanhin
            koko_ -= vanhin_->size();
            vanhin_->tyhjenna();

            vanhin_ = vanhin_->seuraava();
            vanhin_->asetaEdellinen(nullptr);

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

void LiiteCache::karkeen(CacheLiite *liite)
{
    if( liite == uusin_ )
        return;

    if( !vanhin_ ) {
        vanhin_ = liite;
    }

    if( liite->edellinen())
        liite->edellinen()->asetaSeuraava(liite->seuraava() );
    if( liite->seuraava() )
        liite->seuraava()->asetaEdellinen(liite->edellinen() );

    if( liite == vanhin_ && liite->seuraava() ) {
        vanhin_ = liite->seuraava();
    }

    if( uusin_ )
        uusin_->asetaSeuraava(liite);
    liite->asetaEdellinen(uusin_);
    liite->asetaSeuraava(nullptr);
    uusin_ = liite;

/*
    QList<CacheLiite*> lista;
    int lkm = 0;
    int lukossa = 0;
    CacheLiite* ptr = vanhin_;
    while( ptr ) {
        lista.append(ptr);
        lkm++;
        if( ptr->lukossa()) lukossa++;
        ptr = ptr->seuraava();
    }
    qDebug() << " LISTALLA " << lkm << " LUKOSSA " << lukossa << lista;
*/
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
    vanhin_ = nullptr;
}

void LiiteCache::lisaaTallennettu(int liiteId, CacheLiite *liite)
{
    liitteet_.insert(liiteId, liite);
    karkeen( liite );
}

void LiiteCache::poistaPoistettu(int liiteId)
{
    liitteet_.remove(liiteId);
}
