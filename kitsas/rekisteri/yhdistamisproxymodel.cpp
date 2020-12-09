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
#include "yhdistamisproxymodel.h"
#include "laskutus/asiakkaatmodel.h"

YhdistamisProxyModel::YhdistamisProxyModel(QObject* parent) :
    QSortFilterProxyModel(parent)
{
    setSortRole(AsiakkaatModel::NimiRooli);

}

void YhdistamisProxyModel::asetaId(int id)
{
    id_ = id;
}

void YhdistamisProxyModel::suodataNimella(const QString &nimi)
{
    QStringList raakalista = nimi.split(QRegularExpression("\\W+", QRegularExpression::UseUnicodePropertiesOption));
    nimiPalat_.clear();
    for(QString ehdokas : raakalista) {
        if( ehdokas.length() > 2)
            nimiPalat_.append(ehdokas);
    }
    invalidateFilter();
}

bool YhdistamisProxyModel::filterAcceptsRow(int source_row, const QModelIndex &source_parent) const
{
    QModelIndex idx = sourceModel()->index(source_row, 0, source_parent);
    if( idx.data(AsiakkaatModel::IdRooli).toInt() == id_ )
        return false;
    if( nimiPalat_.isEmpty())
        return true;
    QString nimi = idx.data(AsiakkaatModel::NimiRooli).toString();
    for(QString pala : nimiPalat_) {
        if( nimi.contains(pala, Qt::CaseInsensitive))
            return true;
    }
    return false;
}

