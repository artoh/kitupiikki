/*
   Copyright (C) 2019 Arto Hyvättinen

   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program. If not, see <http://www.gnu.org/licenses/>.
*/
#include "kayttooikeusmodel.h"
#include "db/kirjanpito.h"
#include "pilvi/pilvimodel.h"

#include <QVariantMap>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QApplication>



KayttooikeusModel::KayttooikeusModel(QObject *parent)
    : QAbstractListModel(parent)
{
}

int KayttooikeusModel::rowCount(const QModelIndex &parent) const
{
    // For list models only the root node (an invalid parent) should return the list's size. For all
    // other (valid) parents, rowCount() should return 0 so that it does not become a tree model.
    if (parent.isValid())
        return 0;

    return kayttajat_.count();
}

QVariant KayttooikeusModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid())
        return QVariant();

    const Kayttaja& kayttaja = kayttajat_.at(index.row());

    switch (role) {
    case Qt::DisplayRole:
    case NimiRooli:
        return kayttaja.nimi();
    case EmailRooli:
        return kayttaja.email();
    case OikeusRooli:
        return kayttaja.oikeudet();
    case Qt::DecorationRole:
        if( kayttaja.oikeudet().contains("Om"))
            return QIcon(":/pic/yrittaja.png");
        else
            return QIcon(":/pic/mies.png");
    }

    return QVariant();
}

void KayttooikeusModel::paivita()
{
    KpKysely* kysely = kpk( QString("%1/permissions/%2")
                            .arg(kp()->pilvi()->pilviLoginOsoite())
                            .arg(kp()->pilvi()->pilviId()));
    connect(kysely, &KpKysely::vastaus, this, &KayttooikeusModel::listaSaapuu);
    kysely->kysy();
}

QModelIndex KayttooikeusModel::lisaa(const QString &email, const QString nimi)
{
    beginInsertRows(QModelIndex(), kayttajat_.count(), kayttajat_.count());
    kayttajat_.append(Kayttaja(email, nimi));
    endInsertRows();
    return index(kayttajat_.count()-1,0);
}

void KayttooikeusModel::listaSaapuu(QVariant *data)
{
    beginResetModel();

    kayttajat_.clear();
    for(auto item : data->toList())
        kayttajat_.append(Kayttaja(item));

    endResetModel();
}

KayttooikeusModel::Kayttaja::Kayttaja(const QVariant &data)
{
    QVariantMap map = data.toMap();
    nimi_ = map.value("name").toString();
    email_ = map.value("email").toString();
    QVariantList list = map.value("rights").toList();
    for(auto item: list) {
        oikeudet_.append(item.toString());
    }
}

KayttooikeusModel::Kayttaja::Kayttaja(const QString &email, const QString &nimi)
{
    email_ = email;
    nimi_ = nimi;
}

QString KayttooikeusModel::Kayttaja::nimi() const
{
    return nimi_;
}


QString KayttooikeusModel::Kayttaja::email() const
{
    return email_;
}

QStringList KayttooikeusModel::Kayttaja::oikeudet() const
{
    return oikeudet_;
}
