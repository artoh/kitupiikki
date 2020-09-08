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
#include "raporttimuokkausmodel.h"
#include "db/kielikentta.h"

#include <QFont>
#include <QRegularExpression>

RaporttiMuokkausModel::RaporttiMuokkausModel(QObject *parent)
    : QAbstractTableModel(parent)
{
}

QVariant RaporttiMuokkausModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if( role == Qt::DisplayRole && orientation == Qt::Horizontal) {
        switch (section) {
        case TEKSTI: return tr("Teksti");
        case TYYPPI: return tr("Tyyppi");
        case KAAVA: return tr("Kaava");
        }
    }
    return QVariant();
}

int RaporttiMuokkausModel::rowCount(const QModelIndex &parent) const
{
    if (parent.isValid())
        return 0;

    return rivit_.count();
}

int RaporttiMuokkausModel::columnCount(const QModelIndex &parent) const
{
    if (parent.isValid())
        return 0;

    return 3;
}

QVariant RaporttiMuokkausModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid())
        return QVariant();

    QVariantMap map = rivit_.value(index.row()).toMap();

    if( role == Qt::DisplayRole) {
        switch (index.column()) {
        case TEKSTI: {
            int sisennys = map.value("S").toInt();
            QString sstr;
            for(int i=0; i < sisennys; i++)
                sstr.append("    ");
            KieliKentta kk(map);
            return sstr + kk.teksti();
        }
        case TYYPPI: {
            QString kaava = map.value("L").toString();
            if( kaava.contains('H',Qt::CaseInsensitive) )
                return tr("Otsikko");
            if( kaava.contains('*'))
                return tr("Eritelty summa");
            else if(kaava.contains("=") && !kaava.contains("=="))
                return tr("Välisumma");
            else if( map.value("L").toString().isEmpty())
                return tr("Otsikko");
            else
                return tr("Summa");
        }
        case KAAVA:
            return map.value("L");

        }
    }

    if( role == Qt::FontRole && map.value("M").toString().contains("bold")) {
        QFont font;
        font.setBold(true);
        return font;
    }

    if( role == Qt::EditRole)
        return map;

    // FIXME: Implement me!
    return QVariant();
}

bool RaporttiMuokkausModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
    if( !index.isValid())
        return false;

    if( role != Qt::EditRole)
        return false;

    int rivi = index.row();

    rivit_[rivi] = value.toMap();
    emit dataChanged(this->index(rivi,TEKSTI), this->index(rivi,KAAVA));
    return true;
}

void RaporttiMuokkausModel::lisaaRivi(int indeksi, const QVariantMap &data)
{
    beginInsertRows(QModelIndex(), indeksi, indeksi);
    rivit_.insert(indeksi, data);
    endInsertRows();
}

void RaporttiMuokkausModel::poistaRivi(int indeksi)
{
    beginRemoveRows(QModelIndex(), indeksi, indeksi);
    rivit_.removeAt(indeksi);
    endRemoveRows();
}

void RaporttiMuokkausModel::lataa(const QVariantList &lista)
{
    beginResetModel();
    rivit_ = lista;
    endResetModel();
}
