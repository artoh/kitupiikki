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
#include "huoneistolaskutusmodel.h"
#include "../yksikkomodel.h"
#include "../tuotemodel.h"
#include "../tuote.h"
#include "db/kitsasinterface.h"

HuoneistoLaskutusModel::HuoneistoLaskutusModel(KitsasInterface *kitsas, QObject *parent)
    : QAbstractTableModel(parent), kitsas_(kitsas), yksikot_(new YksikkoModel(this))
{
}

QVariant HuoneistoLaskutusModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    // FIXME: Implement me!
    if( role == Qt::DisplayRole && orientation == Qt::Horizontal) {
        switch (section) {
        case NIMI: return tr("Nimike");
        case MAARA: return tr("Määrä");
        case YKSIKKO: return tr("Yksikkö");
        }
    }
    return QVariant();
}

int HuoneistoLaskutusModel::rowCount(const QModelIndex &parent) const
{
    // For list models only the root node (an invalid parent) should return the list's size. For all
    // other (valid) parents, rowCount() should return 0 so that it does not become a tree model.
    if (parent.isValid())
        return 0;

    return laskutettavat_.count();
}

int HuoneistoLaskutusModel::columnCount(const QModelIndex &parent) const
{
    if(parent.isValid())
        return 0;

    return 3;
}

QVariant HuoneistoLaskutusModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid())
        return QVariant();

    // FIXME: Implement me!
    const HuoneistoLaskutettava& l = laskutettavat_.at(index.row());

    if( role == Qt::DisplayRole ) {
        switch (index.column()) {
        case NIMI: {
            const Tuote& tuote = kitsas_->tuotteet()->tuote( l.tuoteId() );
            return tuote.nimike();
        }
        case MAARA:
            return l.lkm();
        case YKSIKKO:
            const Tuote& tuote = kitsas_->tuotteet()->tuote( l.tuoteId() );
            if( tuote.unKoodi().isEmpty())
                return tuote.yksikko();
            else return yksikot_->nimi(tuote.unKoodi());
        }
    }
    if( role == Qt::EditRole && index.column() == MAARA) {
        return l.lkm();
    }

    return QVariant();
}

bool HuoneistoLaskutusModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
    if( index.column() == MAARA && role == Qt::EditRole ) {
        laskutettavat_[index.row()].setLkm( value.toString() );
        return true;
    }
    return false;
}

Qt::ItemFlags HuoneistoLaskutusModel::flags(const QModelIndex &index) const
{
    if( !index.isValid())
        return Qt::NoItemFlags;
    if( index.row() == MAARA)
        return QAbstractTableModel::flags(index) | Qt::ItemIsEditable;
    else
        return QAbstractTableModel::flags(index) | Qt::ItemIsEnabled;
}

void HuoneistoLaskutusModel::lataa(const QVariantList &lista)
{
    beginResetModel();
    for( const auto& item : lista)
        laskutettavat_.append( HuoneistoLaskutettava(item.toMap()) );
    endResetModel();
}

QVariantList HuoneistoLaskutusModel::list() const
{
    QVariantList lista;
    for( const auto& item : laskutettavat_)
        lista.append( item.map());
    return lista;
}

void HuoneistoLaskutusModel::lisaaTuote(int tuote)
{
    beginInsertRows(QModelIndex(), rowCount(), rowCount());
    laskutettavat_.append( HuoneistoLaskutettava(tuote, "1"));
    endInsertRows();
}

void HuoneistoLaskutusModel::poistaRivi(int rivi)
{
    beginRemoveRows(QModelIndex(), rivi, rivi);
    laskutettavat_.removeAt(rivi);
    endRemoveRows();
}

HuoneistoLaskutusModel::HuoneistoLaskutettava::HuoneistoLaskutettava()
{

}

HuoneistoLaskutusModel::HuoneistoLaskutettava::HuoneistoLaskutettava(int tuoteId, const QString &lkm) :
    tuoteId_(tuoteId), lkm_(lkm)
{

}

HuoneistoLaskutusModel::HuoneistoLaskutettava::HuoneistoLaskutettava(const QVariantMap &map)
{
    tuoteId_ = map.value("id").toInt();
    lkm_ = map.value("lkm").toString();
}

QVariantMap HuoneistoLaskutusModel::HuoneistoLaskutettava::map() const
{
    QVariantMap map;
    map.insert("id", tuoteId_);
    map.insert("lkm", lkm_);
    return map;
}
