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
#include "kieli/monikielinen.h"
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

    return maksutavat_.count() + 1;
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

    if( index.row() == maksutavat_.count()) {
        if( role == Qt::DisplayRole || role == Qt::EditRole) {
            if( index.column() == NIMI)
                return tr("Kaikki vastatilit");
        } else if (role == Qt::DecorationRole)
            return QIcon(":/pic/tyhja.png");        

        return QVariant();
    }
    
    const Maksutapa& tapa = maksutavat_.at(index.row());
    
    if( role == Qt::DisplayRole || role == Qt::EditRole) {
        if( index.column() == NIMI) {
            return tapa.nimi();
        } else if( index.column() == TILI) {
            return kp()->tilit()->tiliNumerolla( tapa.tili() ).nimiNumero();
        } else if( index.column() == ERA) {
            if( tapa.uusiEra() )
                return "X";
        }
    } else if( role == Qt::DecorationRole && index.column() == NIMI) {
        return QIcon(":/maksutavat/" + tapa.kuva() + ".png");
    } else if( role == TiliRooli) {
        return tapa.tili();
    } else if( role == UusiEraRooli) {
        return tapa.uusiEra();
    }

    return QVariant();
}

Maksutapa MaksutapaModel::maksutapa(int indeksi) const
{
    return maksutavat_.value(indeksi);
}

void MaksutapaModel::lisaaRivi(int indeksi, const Maksutapa &maksutapa)
{
    beginInsertRows(QModelIndex(), indeksi, indeksi);
    maksutavat_.insert(indeksi, maksutapa);
    endInsertRows();
    tallenna();
}

void MaksutapaModel::muutaRivi(int indeksi, const Maksutapa &maksutapa)
{
    maksutavat_[indeksi] = maksutapa;
    emit dataChanged(index(indeksi,0), index(indeksi,3));
    tallenna();
}

void MaksutapaModel::poistaRivi(int indeksi)
{
    beginRemoveRows(QModelIndex(), indeksi, indeksi);
    maksutavat_.removeAt(indeksi);
    endRemoveRows();
    tallenna();
}

void MaksutapaModel::siirra(int mista, int minne)
{
    maksutavat_.move(mista, minne);
    emit dataChanged(index(mista,0), index(minne,ERA));
    tallenna();
}

void MaksutapaModel::lataa(int tuloVaiMeno)
{
    tuloVaiMeno_ = tuloVaiMeno;
    const QString& muoto = kp()->asetukset()->asetus(AsetusModel::Muoto);

    beginResetModel();
    QVariantList lista = QJsonDocument::fromJson( kp()->asetukset()->asetus( tuloVaiMeno == MENO ? "maksutavat-" : "maksutavat+" ).toUtf8() ).toVariant().toList();
    for(const auto& item: lista) {
        Maksutapa maksutapa(item.toMap());
        const QString& muotoEhto = maksutapa.muotoehto();
        if( !muotoEhto.isEmpty() && muotoEhto != muoto)
            continue;
        maksutavat_.append(maksutapa);
    }
    endResetModel();
}

void MaksutapaModel::tallenna()
{
    QVariantList lista;
    for(const auto& tapa: maksutavat_)
        lista.append(tapa.map());

    QString json = QString::fromUtf8(QJsonDocument::fromVariant( lista ).toJson());
    if( tuloVaiMeno_ == MENO)
        kp()->asetukset()->aseta("maksutavat-", json);
    else if( tuloVaiMeno_ == TULO)
        kp()->asetukset()->aseta("maksutavat+", json);
}
