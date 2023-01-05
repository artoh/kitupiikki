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

    return bannerit_.count() + 1;
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
            case IndeksiRooli:
                return -1;
            default:
                return QVariant();
        }
    }

    const Banneri banneri = bannerit_.at(index.row() - 1);

    switch (role) {
    case IdRooli:
        return banneri.id();
    case NimiRooli:
        return banneri.nimi();
    case KuvaRooli:
        return banneri.kuvake();
    default:
        return QVariant();
    }

}

void BannerModel::lisaa(const QString &nimi, const QImage &kuva)
{
    const QString id = QUuid::createUuid().toString();

    beginInsertRows(QModelIndex(), rowCount(), rowCount());
    bannerit_.append(Banneri(id, nimi, kuva));
    endInsertRows();

    kp()->asetukset()->aseta(ASETUSPOLKU.arg(id), nimi);

    QByteArray ba;
    QBuffer buffer(&ba);
    buffer.open(QIODevice::WriteOnly);
    kuva.save(&buffer,"JPEG",80);
    buffer.close();

    KpKysely* kysely = kpk(QString(KYSELYPOLKU.arg(id)), KpKysely::PUT);
    kysely->lahetaTiedosto(ba);

}

void BannerModel::muuta(const QString id, const QString &nimi, const QImage &kuva)
{
    for(int i=0; i < bannerit_.count(); i++) {
        if(bannerit_.at(i).id() == id) {
            bannerit_[i].asetaNimi(nimi);
            bannerit_[i].asetaKuva(kuva);
            emit dataChanged(index(i+1), index(i+1));

            kp()->asetukset()->aseta(ASETUSPOLKU.arg(id), nimi);

            QByteArray ba;
            QBuffer buffer(&ba);
            buffer.open(QIODevice::WriteOnly);
            kuva.save(&buffer,"JPEG",80);
            buffer.close();

            KpKysely* kysely = kpk(QString(KYSELYPOLKU.arg(id)), KpKysely::PUT);
            kysely->lahetaTiedosto(ba);
            return;
        }
    }

}

void BannerModel::poista(const QString id)
{
    for(int i=0; i < bannerit_.count(); i++) {
        if( bannerit_.at(i).id() == id) {
            beginRemoveRows(QModelIndex(), i, i);
            bannerit_.removeAt(i);
            endRemoveRows();
            kp()->asetukset()->poista(ASETUSPOLKU.arg(id));
        }
    }
}

void BannerModel::lataa()
{
    beginResetModel();

    bannerit_.clear();
    latausLista_.clear();

    const int avainpituus = ASETUSPOLKU.length();
    for(auto avain : kp()->asetukset()->avaimet(ASETUSPOLKU.arg(QString()))) {
        const QString id = avain.mid(avain.lastIndexOf("/") + 1);
        const QString nimi = kp()->asetukset()->asetus(avain);

        if( nimi.isEmpty()) continue;

        bannerit_.append(Banneri(id, nimi));
        latausLista_.append(id);
    }
    endResetModel();

    lataaKuva();
}

QImage BannerModel::kuva(const QString uuid) const
{
    for(int i=0; i < bannerit_.count(); i++) {
        if( bannerit_.at(i).id() == uuid) {
            return bannerit_.at(i).kuva();
        }
    }
    return QImage();
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

    for(int i=0; i < bannerit_.count(); i++) {
        if(bannerit_.at(i).id() == uuid) {
            bannerit_[i].asetaKuva(kuva);
            emit dataChanged(index(i+1), index(i+1));
            break;
        }
    }
    lataaKuva();
}

const QString BannerModel::ASETUSPOLKU = "Laskutus/Banneri/%1";
const QString BannerModel::KYSELYPOLKU = "/liitteet/0/banner-%1";

BannerModel::Banneri::Banneri()
{

}

BannerModel::Banneri::Banneri(const QString &id, const QString &nimi, const QImage &kuva) :
    id_{id}, nimi_{nimi}, kuva_{kuva}
{

}

QPixmap BannerModel::Banneri::kuvake() const
{
    QImage scaled = kuva_.scaledToWidth(800, Qt::SmoothTransformation);
    return QPixmap::fromImage(scaled);
}

void BannerModel::Banneri::asetaKuva(const QImage &image)
{
    kuva_ = image;
}

void BannerModel::Banneri::asetaNimi(const QString &nimi)
{
    nimi_ = nimi;
}
