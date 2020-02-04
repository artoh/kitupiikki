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
#include "kiertomuokkausmodel.h"
#include "model/tosite.h"

#include <QIcon>

KiertoMuokkausModel::KiertoMuokkausModel(QObject *parent)
    : QAbstractTableModel(parent)
{
}

QVariant KiertoMuokkausModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if( orientation == Qt::Horizontal && role == Qt::DisplayRole) {
        switch (section) {
        case NIMI: return tr("Osallistuja");
        case ROOLI: return tr("Vaiheen jälkeen");
        case ILMOITA: return tr("Ilmoitus");
        }
    }
    return QVariant();
}


int KiertoMuokkausModel::rowCount(const QModelIndex &parent) const
{
    if (parent.isValid())
        return 0;

    return lista_.count();
}

int KiertoMuokkausModel::columnCount(const QModelIndex &parent) const
{
    if (parent.isValid())
        return 0;

    return 3;
}

QVariant KiertoMuokkausModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid())        
        return QVariant();

    if( (role == Qt::DisplayRole) | (role == Qt::EditRole)) {
        const QVariantMap& map = lista_.at(index.row()).toMap();
        switch (index.column()) {
        case NIMI:
            if( map.value("nimi").toString().isEmpty())
                return nimicache_.value(map.value("userid").toInt());
            return map.value("nimi");
        case ROOLI:
            if( role == Qt::DisplayRole)
                return Tosite::tilateksti(map.value("rooli").toInt());
            else
                return map.value("rooli").toInt();
        case ILMOITA:
            if( role == Qt::DisplayRole) {
                if( map.value("ilmoita").toBool())
                    return tr("Sähköpostilla");
            } else
                return map.value("ilmoita").toBool();
        }
    } else if( role == Qt::DecorationRole) {
        const QVariantMap& map = lista_.at(index.row()).toMap();
        if( index.column() == ROOLI) {
            switch (map.value("rooli").toInt()) {
            case Tosite::SAAPUNUT: return QIcon(":/pic/inbox.png");
            case Tosite::TARKASTETTU: return QIcon(":/pixaby/tarkastettu.svg");
            case Tosite::HYVAKSYTTY: return QIcon(":/pixaby/hyvaksytty.svg");
            }
        }
    } else if( role == RooliRooli) {
        return lista_.at(index.row()).toMap().value("rooli");
    }



    return QVariant();
}

bool KiertoMuokkausModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
    if (data(index, role) != value) {
        QVariantMap map = lista_.at(index.row()).toMap();
        map.insert("ilmoita", value.toBool());
        lista_[index.row()]=map;
        emit dataChanged(index, index, QVector<int>() << role);
        return true;
    }
    return false;
}

Qt::ItemFlags KiertoMuokkausModel::flags(const QModelIndex &index) const
{
    if (!index.isValid())
        return Qt::NoItemFlags;

    if( index.column() == ILMOITA)
        return Qt::ItemIsEnabled | Qt::ItemIsEditable;
    return Qt::ItemIsEnabled;
}

void KiertoMuokkausModel::lisaaRivi(const QVariantMap &rivi)
{
    beginInsertRows(QModelIndex(), rowCount(), rowCount());
    lista_.append(rivi);
    endInsertRows();
}

void KiertoMuokkausModel::poistaRivi(int indeksi)
{
    beginRemoveRows(QModelIndex(), indeksi, indeksi);
    lista_.removeAt(indeksi);
    endRemoveRows();
}

void KiertoMuokkausModel::lataa(const QVariantList &lista)
{
    beginResetModel();
    lista_ = lista;
    endResetModel();
}

void KiertoMuokkausModel::lisaaNimet(const QMap<int, QString> nimet)
{
    nimicache_ = nimet;
    emit dataChanged(index(0,NIMI),index(rowCount()-1, NIMI));
}


QVariantList KiertoMuokkausModel::kiertoLista() const
{
    return lista_;
}
