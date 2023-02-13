#include "liite.h"


#include "db/kirjanpito.h"
#include "liitecache.h"

#include "liitteetmodel.h"
#include "db/kirjanpito.h"

#include <QVariantMap>
#include <QMessageBox>

/*
Liite::Liite() :
    cache_{new CacheLiite()}
{

}
*/

Liite::Liite(LiitteetModel *model, const QVariantMap &map) :
    model_(model)
{
    liiteId_ = map.value("id").toInt();
    nimi_ = map.value("nimi").toString();
    rooli_ = map.value("roolinimi").toString();
    tyyppi_ = map.value("tyyppi").toString();
    cache_ = kp()->liiteCache()->liite(liiteId_);
    cache_->lukitse();
}

Liite::Liite(LiitteetModel *model, const QByteArray &ba, const QString &polku, const QString& rooli) :
    model_(model)
{
    QFileInfo info(polku);
    tila_ = TALLENNETTAVA;

    liiteId_ = 0;
    nimi_ = info.fileName();
    rooli_ = rooli;    
    tyyppi_ = KpKysely::tiedostotyyppi(ba);
    cache_ = new CacheLiite();
    cache_->lukitse();
    cache_->setData(ba);

}

/*
Liite::Liite(const Liite &liite)
{
    liiteId_ = liite.liiteId_;
    nimi_ = liite.nimi_;
    rooli_ = liite.rooli_;
    tyyppi_ = liite.tyyppi_;
    tila_ = liite.tila_;
    cache_ = liite.cache_;
    cache_->lukitse();
}
*/

Liite::~Liite()
{
    cache_->vapauta();
}

bool Liite::kaytettavissa() const
{
    return cache_->kaytettavissa();
}

Liite::LiiteTila Liite::tila() const
{
    if( tila_ == HAETAAN && kaytettavissa()) {
        return HAETTU;
    } else {
        return tila_;
    }
}

QPixmap Liite::thumb() const
{
    return cache_->thumb();
}

QByteArray *Liite::dataPtr() const
{
    return cache_->dataPtr();
}

void Liite::liita(bool ocr)
{
    vaihdaTila(LIITETAAN);
    KpKysely* liitekysely = kpk("/liitteet", KpKysely::POST);

    if(ocr)
        liitekysely->lisaaAttribuutti("ocr", "json");

    connect( liitekysely, &KpKysely::lisaysVastaus, this, &Liite::liitetty);
    connect( liitekysely, &KpKysely::virhe, this, &Liite::tallennusVirhe);

    QMap<QString,QString> meta;
    meta.insert("Filename", nimi());
    meta.insert("Content-type", tyyppi());
    liitekysely->lahetaTiedosto( *dataPtr(), meta);
}

void Liite::tallenna(int tositeId)
{
    KpKysely* liitekysely = rooli().isEmpty() ?
                kpk(QString("/liitteet/%1").arg(tositeId), KpKysely::POST) :
                kpk(QString("/liitteet/%1/%2").arg(tositeId).arg(rooli()), KpKysely::POST);

    connect( liitekysely, &KpKysely::lisaysVastaus, this, &Liite::tallennettu);
    connect( liitekysely, &KpKysely::virhe, this, &Liite::tallennusVirhe);

    QMap<QString,QString> meta;
    meta.insert("Filename", nimi());
    meta.insert("Content-type", tyyppi());
    liitekysely->lahetaTiedosto( *dataPtr(), meta);

}

void Liite::liitetty(const QVariant &reply, int lisattyId)
{
    liiteId_ = lisattyId;
    vaihdaTila(LIITETTY);

    QVariantMap map = reply.toMap();
    if(!map.isEmpty())
        model_->ocr(map);
}

void Liite::tallennettu(const QVariant & /*reply*/, int lisattyId)
{
    liiteId_ = lisattyId;
    vaihdaTila(TALLENNETTU);
}



void Liite::tallennusVirhe(int virhe, const QString selitys)
{
    QMessageBox::critical(nullptr, tr("Liitteen tallentaminen epäonnistui"),
                                   tr("Liitteen tallentamisessa tapahtui virhe %1 : %2")
                          .arg(virhe).arg(selitys));
}

void Liite::vaihdaTila(LiiteTila uusiTila)
{
    tila_ = uusiTila;
    model_->liitteenTilaVaihtui(uusiTila);
}



