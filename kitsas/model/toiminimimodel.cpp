#include "toiminimimodel.h"

#include "db/kirjanpito.h"
#include "db/asetusmodel.h"

#include <QBuffer>
#include <QJsonDocument>

ToiminimiModel::ToiminimiModel(Kirjanpito *parent)
    : QAbstractListModel(parent)
{
}

int ToiminimiModel::rowCount(const QModelIndex &parent) const
{
    // For list models only the root node (an invalid parent) should return the list's size. For all
    // other (valid) parents, rowCount() should return 0 so that it does not become a tree model.
    if (parent.isValid())
        return 0;

    return lista_.count() + 1;
}

QVariant ToiminimiModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid())
        return QVariant();

    // FIXME: Implement me!
    if( role == Logo ) {
        return QPixmap::fromImage(logo(index.row()));
    } else if( role == Nakyva) {
        return tieto(Piilossa, index.row()).isEmpty() ? "X" : "";
    } else if( role == Indeksi) {
        return index.row();
    } else if( role == Nimi || role > Qt::UserRole) {
        return tieto(role, index.row());
    }

    return QVariant();
}

bool ToiminimiModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
    int rivi = index.row();
    aseta(rivi, role, value.toString());
    return true;
}

void ToiminimiModel::lataa()
{
    beginResetModel();
    const QString& data = kp()->asetukset()->asetus(AsetusModel::Toiminimet);
    QVariantList lista = QJsonDocument::fromJson(data.toUtf8()).toVariant().toList();
    lista_.clear();
    logo_.clear();

    lista_.reserve(lista.count());
    logo_.resize(lista.count());
    for(auto& item : lista) {
        lista_.append( item.toMap() );
    }

    if( kp()->yhteysModel()) {
        for(int i=0; i < lista_.count(); i++) {
                KpKysely *logokysely = kpk(QString("/liitteet/0/logo-%1").arg(i+1));
                connect( logokysely, &KpKysely::vastaus, this, [this,i] (QVariant* vastaus)  { this->logoSaapui(i, vastaus); });
                logokysely->kysy();
        }
    }
    endResetModel();
}

void ToiminimiModel::tallenna()
{
    kp()->asetukset()->aseta(AsetusModel::Toiminimet, toString());
}

int ToiminimiModel::lisaaToiminimi(const QString &toiminimi)
{
    beginInsertRows(QModelIndex(), lista_.size(), lista_.size());
    QVariantMap uusi;
    uusi.insert( avaimet__.at(Nimi), toiminimi );
    uusi.insert( avaimet__.at(Katuosoite), tieto(Katuosoite));
    uusi.insert( avaimet__.at(Postinumero), tieto(Postinumero));
    uusi.insert( avaimet__.at(Kaupunki), tieto(Kaupunki));
    uusi.insert( avaimet__.at(Puhelin), tieto(Puhelin));
    uusi.insert( avaimet__.at(Sahkoposti), tieto(Sahkoposti));
    uusi.insert( avaimet__.at(Kotisivu), tieto(Kotisivu));
    uusi.insert( avaimet__.at(LogonSijainti), tieto(LogonSijainti));
    lista_.append(uusi);
    const int uusiIndeksi = lista_.size();
    logo_.resize( uusiIndeksi );
    asetaLogo(uusiIndeksi, logo());
    endInsertRows();
    return uusiIndeksi;
}

QString ToiminimiModel::toString() const
{
    QVariantList lista;
    for(int i=0; i < lista_.count(); i++) {
        lista.append( lista_.at(i) );
    }
    return QJsonDocument::fromVariant(lista).toJson(QJsonDocument::Compact);
}

QString ToiminimiModel::tieto(int rooli, int indeksi) const
{
    const QString& avain = avaimet__.at(rooli);
    if( indeksi == 0) {
        return kp()->asetukset()->asetus(avain);
    }
    return lista_.value(indeksi-1).value(avain).toString();
}

void ToiminimiModel::aseta(int indeksi, int rooli, const QString &tieto)
{
    const QString& avain = avaimet__.at(rooli);
    if( indeksi == 0) {
        kp()->asetukset()->aseta(avain, tieto);
    } else {
        lista_[indeksi-1].insert(avain, tieto);
    }
    emit dataChanged(index(indeksi),index(indeksi));
}

QImage ToiminimiModel::logo(int indeksi) const
{
    if( indeksi == 0)
        return kp()->logo();
    else
        return logo_.value(indeksi-1);
}

void ToiminimiModel::asetaLogo(int indeksi, const QImage &logo)
{
    if( indeksi == 0) {
        kp()->asetaLogo(logo);
    } else {
        logo_[indeksi-1] = logo;

        QByteArray ba;

        QBuffer buffer(&ba);
        buffer.open(QIODevice::WriteOnly);
        if(!logo.isNull())
            logo.save(&buffer, "PNG");
        buffer.close();

        KpKysely *kysely = kpk(QString("/liitteet/0/logo-%1").arg(indeksi), KpKysely::PUT);
        kysely->lahetaTiedosto(ba);
    }
    emit dataChanged(index(indeksi),index(indeksi));
}

void ToiminimiModel::logoSaapui(int indeksi, QVariant *reply)
{
    QByteArray ba = reply->toByteArray();

    QImage logo = QImage::fromData( ba );
    logo_[indeksi] = logo;
}


std::map<int,QString> ToiminimiModel::avaimet__ = {
    { Nimi, "Nimi" },
    { Katuosoite, "Katuosoite" },
    { Postinumero, "Postinumero"},
    { Kaupunki, "Kaupunki"} ,
    { Puhelin, "Puhelin"} ,
    { Sahkoposti, "Email"},
    { Kotisivu, "Kotisivu"},
    { LogonSijainti, "LogonSijainti"},
    { Piilossa, "Piilossa"},
};
