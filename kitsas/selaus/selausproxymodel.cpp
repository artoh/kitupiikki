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
#include "selausproxymodel.h"


SelausProxyModel::SelausProxyModel(SelausModel *model, QObject *parent)
    : QSortFilterProxyModel(parent), model_(model)
{
    setSourceModel(model);
    setSortCaseSensitivity(Qt::CaseInsensitive);
    setSortRole(Qt::EditRole);
}

void SelausProxyModel::suodataTililla(int tilinumero)
{
    if( tilinumero != tiliSuodatus_) {
        tiliSuodatus_ = tilinumero;
        invalidateFilter();
    }
}

void SelausProxyModel::etsi(const QString &teksti)
{
    if( teksti != etsiSuodatus_) {

        etsiSuodatus_ = teksti;
        etsiSuodatusKaytossa_ = !teksti.isEmpty();
        invalidateFilter();
    }
}

bool SelausProxyModel::filterAcceptsRow(int source_row, const QModelIndex & /*source_parent */) const
{
    if( tiliSuodatus_ ) {
        if( model_->tili(source_row) != tiliSuodatus_)
            return false;
    }
    if( etsiSuodatusKaytossa_) {
        if( !model_->etsiTeksti(source_row).contains(etsiSuodatus_))
            return false;
    }
    return true;
}
