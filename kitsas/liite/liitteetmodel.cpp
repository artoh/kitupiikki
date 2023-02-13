#include "liitteetmodel.h"
#include "db/kirjanpito.h"
#include "liite/cacheliite.h"
#include "liite/liitecache.h"



#include <QBuffer>
#include <QImage>
#include <QVector>
#include <QByteArray>
#include <QFile>
#include <QMessageBox>
#include <QPdfDocument>


LiitteetModel::LiitteetModel(QObject *parent)
    : QAbstractListModel(parent),
      puskuri_{new QBuffer()},
      pdfDoc_{new QPdfDocument(this)}
{
    connect( kp()->liiteCache(), &LiiteCache::liiteHaettu,
             this, &LiitteetModel::valimuistiLiite);
}

LiitteetModel::~LiitteetModel()
{
    for(auto ptr: liitteet_)
        delete ptr;
    liitteet_.clear();
}

int LiitteetModel::rowCount(const QModelIndex &parent) const
{
    // For list models only the root node (an invalid parent) should return the list's size. For all
    // other (valid) parents, rowCount() should return 0 so that it does not become a tree model.
    if (parent.isValid())
        return 0;

    // FIXME: Implement me!
    return liitteet_.count();
}

QVariant LiitteetModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid())
        return QVariant();

    // FIXME: Implement me!
    const Liite* liite = liitteet_.at(index.row());

    switch( role ) {
        case Qt::DisplayRole:
            if(liite->nimi().isEmpty())
                return liite->rooli();
            else
                return liite->nimi();
        case Qt::DecorationRole:
            if( liite->thumb().isNull() )
                return QIcon(":/pic/tekstisivu.png");
            else
                return liite->thumb();
    }

    return QVariant();
}

void LiitteetModel::lataa(const QVariantList &data)
{
    beginResetModel();

    liitteet_.clear();
    naytettavaIndeksi_ = -1;

    for(const auto& item : qAsConst(data)) {
        const QVariantMap& map = item.toMap();
        liitteet_.append(new Liite(this, map));
    }


    endResetModel();

    // TODO: Parhaimman näytettävän tunnistus
    if( interaktiivinen_) {
        if(liitteet_.isEmpty()) {
            nayta(-1);
        } else {
            nayta(0);
        }
    }
}

void LiitteetModel::clear()
{
    beginResetModel();
    for(auto item: liitteet_)
        delete item;
    liitteet_.clear();
    endResetModel();
}

void LiitteetModel::asetaInteraktiiviseksi(bool onko)
{
    interaktiivinen_ = onko;
}

bool LiitteetModel::lisaaHeti(const QByteArray &liite, const QString &polku)
{
    // Esikäsittely


    // Tallennus
    beginInsertRows(QModelIndex(), rowCount(), rowCount());
    liitteet_.append(new Liite(this, liite, polku));
    endInsertRows();


    // Tuonti
    return false;

}

bool LiitteetModel::lisaaHetiTiedosto(const QString &polku)
{
    QByteArray ba;
    QFile tiedosto(polku);
    if( !tiedosto.open(QIODevice::ReadOnly) )
    {
        QMessageBox::critical(nullptr, tr("Tiedostovirhe"),
                              tr("Tiedoston %1 avaaminen epäonnistui \n%2").arg(polku, tiedosto.errorString()));
        return false;
    }

    ba = tiedosto.readAll();
    tiedosto.close();
    return lisaaHeti( ba, polku );
}

void LiitteetModel::nayta(int indeksi)
{
    naytettavaIndeksi_ = indeksi;
    puskuri_->close();

    emit valittuVaihtui(indeksi);

    if( indeksi > -1) {
        if( liitteet_.at(indeksi)->kaytettavissa()) {
            naytaKayttajalle();
        }
    }
}

QByteArray *LiitteetModel::sisalto()
{
    if( naytettavaIndeksi_ < 0)
        return nullptr;
    QByteArray* data = liitteet_.at(naytettavaIndeksi_)->dataPtr();
    return data;
}

void LiitteetModel::valimuistiLiite(int liiteId)
{
    for(int i=0; i < liitteet_.count(); i++) {
        if( liitteet_.at(i)->id() == liiteId) {
            emit dataChanged( createIndex(i,0), createIndex(i,0), QVector<int>() << Qt::DecorationRole );
            if( i == naytettavaIndeksi_ ) {
                naytaKayttajalle();
            }
        }
    }
}

void LiitteetModel::naytaKayttajalle()
{
    if(interaktiivinen_ && naytettavaIndeksi_ > -1 && naytettavaIndeksi_ < liitteet_.count()) {
        QByteArray* data = liitteet_.at(naytettavaIndeksi_)->dataPtr();
        if( !data) return;

        if( data->startsWith("%PDF")) {
            puskuri_->setBuffer(data);
            puskuri_->open(QIODevice::ReadOnly);
            pdfDoc_->load(puskuri_);
            emit naytaPdf();
        } else {
            emit naytaSisalto();
        }
    }
}

void LiitteetModel::tarkastaKaikkiLiitteet()
{

}


