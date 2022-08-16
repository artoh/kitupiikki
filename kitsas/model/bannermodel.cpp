#include "bannermodel.h"

#include "db/kirjanpito.h"
#include "db/asetusmodel.h"

#include <QUuid>
#include <QBuffer>

BannerModel::BannerModel(QObject *parent)
    : QAbstractListModel(parent)
{
}

int BannerModel::rowCount(const QModelIndex &parent) const
{
    // For list models only the root node (an invalid parent) should return the list's size. For all
    // other (valid) parents, rowCount() should return 0 so that it does not become a tree model.
    if (parent.isValid())
        return 0;

    return idt_.count() + 1;
}

QVariant BannerModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid())
        return QVariant();

    if( !index.row()) {
        switch (role) {
            case IdRooli:
                return QString();
            case NimiRooli:
                return tr("Ei banneria");
            case KuvaRooli:
            {
                QImage scaled(800,200,QImage::Format_RGB16);
                scaled.fill(QColor(Qt::white));
                return QPixmap::fromImage(scaled);
            }
            default:
                return QVariant();
        }
    }

    QString avain = idt_.value(index.row() - 1);

    switch (role) {
    case IdRooli:
        return avain;
    case NimiRooli:
        return nimet_.value(avain);
    case KuvaRooli:
    {
        QImage scaled = kuva(avain).scaledToWidth(800, Qt::SmoothTransformation);
        return QPixmap::fromImage(scaled);
    }
    default:
        return QVariant();
    }

}

void BannerModel::lisaa(const QString &nimi, const QImage &kuva)
{
    const QString id = QUuid::createUuid().toString();
    kp()->asetukset()->aseta(ASETUSPOLKU + id, nimi);

    QByteArray ba;
    QBuffer buffer(&ba);
    buffer.open(QIODevice::WriteOnly);
    kuva.save(&buffer,"PNG");
    buffer.close();

    KpKysely* kysely = kpk(QString(KYSELYPOLKU.arg(id)), KpKysely::PUT);
    kysely->lahetaTiedosto(ba);

    nimet_.insert(id, nimi);
    kuvat_.insert(id, kuva);

    beginInsertRows(QModelIndex(), rowCount(), rowCount());
    idt_.append(id);
    endInsertRows();

}

void BannerModel::lataa()
{
    beginResetModel();

    idt_.clear();
    nimet_.clear();
    kuvat_.clear();

    const int avainpituus = ASETUSPOLKU.length();
    for(auto avain : kp()->asetukset()->avaimet(ASETUSPOLKU)) {
        const QString id = avain.mid(avainpituus);
        const QString nimi = kp()->asetukset()->asetus(avain);
        idt_.append( id );
        nimet_.insert(id, nimi);
    }
    endResetModel();

    latausLista_ = idt_;
    lataaKuva();
}

QImage BannerModel::kuva(const QString uuid) const
{
    return kuvat_.value(uuid);
}

void BannerModel::lataaKuva()
{
    if( !latausLista_.isEmpty()) {
        QString uid = latausLista_.takeLast();
        KpKysely* kysely = kpk(KYSELYPOLKU.arg(uid));
        connect( kysely, &KpKysely::vastaus, this, [this, uid](QVariant* vastaus)
            { this->kuvaSaapuu(uid, vastaus);});
        kysely->kysy();
    }
}

void BannerModel::kuvaSaapuu(const QString &uuid, QVariant *reply)
{
    QByteArray ba = reply->toByteArray();
    QImage kuva = QImage::fromData( ba );
    kuvat_.insert(uuid, kuva);

    const int indeksi = idt_.indexOf(uuid);
    if( indeksi > -1) {
        emit dataChanged(index(indeksi), index(indeksi), QVector<int>() << Qt::DecorationRole );
    }
    lataaKuva();
}

const QString BannerModel::ASETUSPOLKU = "Laskutus/Banneri/";
const QString BannerModel::KYSELYPOLKU = "/liitteet/0/banner-%1";
