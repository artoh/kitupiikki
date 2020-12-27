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
#include "maksutapamodel.h"

#include <QJsonDocument>
#include "db/kirjanpito.h"
#include "db/kielikentta.h"
#include <QDebug>

MaksutapaModel::MaksutapaModel(QObject *parent)
    : QAbstractTableModel(parent)
{
}

QVariant MaksutapaModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if( role == Qt::DisplayRole && orientation == Qt::Horizontal) {
        if( section == NIMI)
            return tr("Nimi");
        else if( section == TILI)
            return tr("Tili");
        else
            return tr("Uusi erä");
    }
    return QVariant();
}

int MaksutapaModel::rowCount(const QModelIndex &parent) const
{
    if (parent.isValid())
        return 0;

    return lista_.count() + 1;
}

int MaksutapaModel::columnCount(const QModelIndex &parent) const
{
    if (parent.isValid())
        return 0;

    return 3;
}

QVariant MaksutapaModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid())
        return QVariant();

    if( index.row() == lista_.count()) {
        if( role == Qt::DisplayRole || role == Qt::EditRole) {
            if( index.column() == NIMI)
                return tr("Kaikki vastatilit");
        } else if (role == Qt::DecorationRole)
            return QIcon(":/pic/tyhja.png");        

        return QVariant();
    }

    QVariantMap map = lista_.value(index.row()).toMap();
    if( role == Qt::DisplayRole || role == Qt::EditRole) {
        if( index.column() == NIMI) {
            KieliKentta kk( map );
            return kk.teksti();
        } else if( index.column() == TILI) {
            return kp()->tilit()->tiliNumerolla( map.value("TILI").toInt() ).nimiNumero();
        } else if( index.column() == ERA) {
            if( map.value("ERA").toInt() == -1 )
                return "X";
        }
    } else if( role == Qt::DecorationRole && index.column() == NIMI) {
        return QIcon(":/maksutavat/" + map.value("KUVA").toString() + ".png");
    } else if( role == TiliRooli) {
        return map.value("TILI");
    } else if( role == UusiEraRooli) {
        return map.value("ERA").toInt();
    }

    return QVariant();
}

QVariantMap MaksutapaModel::rivi(int indeksi)
{
    return lista_.value(indeksi).toMap();
}

void MaksutapaModel::lisaaRivi(int indeksi, QVariantMap rivi)
{
    beginInsertRows(QModelIndex(), indeksi, indeksi);
    lista_.insert(indeksi, rivi);
    endInsertRows();
    tallenna();
}

void MaksutapaModel::muutaRivi(int indeksi, QVariantMap rivi)
{
    lista_[indeksi] = rivi;
    emit dataChanged(index(indeksi,0), index(indeksi,3));
    tallenna();
}

void MaksutapaModel::poistaRivi(int indeksi)
{
    beginRemoveRows(QModelIndex(), indeksi, indeksi);
    lista_.removeAt(indeksi);
    endRemoveRows();
    tallenna();
}

void MaksutapaModel::siirra(int mista, int minne)
{
    lista_.move(mista, minne);
    emit dataChanged(index(mista,0), index(minne,ERA));
    tallenna();
}

void MaksutapaModel::lataa(int tuloVaiMeno)
{
    tuloVaiMeno_ = tuloVaiMeno;
    beginResetModel();
    lista_ = QJsonDocument::fromJson( kp()->asetukset()->asetus( tuloVaiMeno == MENO ? "maksutavat-" : "maksutavat+" ).toUtf8() ).toVariant().toList();
    endResetModel();
}

void MaksutapaModel::tallenna()
{
    QString json = QString::fromUtf8(QJsonDocument::fromVariant( lista_ ).toJson());
    if( tuloVaiMeno_ == MENO)
        kp()->asetukset()->aseta("maksutavat-", json);
    else if( tuloVaiMeno_ == TULO)
        kp()->asetukset()->aseta("maksutavat+", json);
}
