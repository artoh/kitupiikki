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
#include "eraproxymodel.h"

#include "eranvalintamodel.h"

EraProxyModel::EraProxyModel(QObject *parent)
    : QSortFilterProxyModel(parent)
{

}

void EraProxyModel::suodata(const QDate &min, const QDate &max, const QString &teksti)
{
    minDate_ = min;
    maxDate_ = max;
    teksti_ = teksti;
    invalidateFilter();
}


bool EraProxyModel::filterAcceptsRow(int source_row, const QModelIndex &source_parent) const
{
    const QModelIndex indeksi = sourceModel()->index(source_row,0,source_parent);

    const QDate& pvm = indeksi.data(EranValintaModel::PvmRooli).toDate();
    if( pvm.isValid() &&
            (pvm < minDate_ || pvm > maxDate_))
        return false;

    const QString& teksti = indeksi.data(EranValintaModel::TekstiRooli).toString();
    if( !teksti_.isEmpty() && !teksti.contains(teksti_, Qt::CaseInsensitive))
        return false;

    return true;
}
