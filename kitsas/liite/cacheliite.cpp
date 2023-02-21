#include "cacheliite.h"

#include <QImage>

CacheLiite::CacheLiite()
{

}

CacheLiite::CacheLiite(LiiteTila tila) :
    tila_{tila}
{

}

void CacheLiite::setData(const QByteArray &data)
{
    data_ = data;        
    tila_ = HAETTU;

    QImage image = QImage::fromData(data).scaled(64,64,Qt::KeepAspectRatio);
    thumb_ = QPixmap::fromImage(image);
}

void CacheLiite::asetaSeuraava(CacheLiite *seuraava)
{
    seuraava_ = seuraava;
}

void CacheLiite::asetaEdellinen(CacheLiite *edellinen)
{
    edellinen_ = edellinen;
}


void CacheLiite::lukitse()
{
    lukko_++;
}

void CacheLiite::vapauta()
{
    if(lukko_) lukko_--;
    if(!lukko_ && tila_ == KELVOTON) {
        data_.clear();
    }
}

bool CacheLiite::kaytettavissa() const
{
    return tila_ == HAETTU;
}

void CacheLiite::tyhjenna()
{
    data_.clear();
    thumb_ = QPixmap();
    tila_ = TYHJENNETTY;
}

QByteArray *CacheLiite::dataPtr()
{
    if( kaytettavissa() && lukossa())
        return &data_;
    else
        return nullptr;
}

QPixmap CacheLiite::thumb() const
{
    return thumb_;
}
