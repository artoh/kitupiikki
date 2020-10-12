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
#include "tositeselausproxymodel.h"



TositeSelausProxyModel::TositeSelausProxyModel(TositeSelausModel *model, QObject* parent)
    : QSortFilterProxyModel(parent), model_(model)
{
    setSourceModel(model);
    setSortCaseSensitivity(Qt::CaseInsensitive);
}

void TositeSelausProxyModel::etsi(const QString &teksti)
{
    if( teksti != etsiSuodatus_) {
        etsiSuodatus_ = teksti;
        invalidateFilter();
    }
}

void TositeSelausProxyModel::suodataTositetyyppi(int tyyppi)
{
    if( tyyppi != tyyppiSuodatus_) {
        tyyppiSuodatus_ = tyyppi;
        sarjaSuodatus_.clear();
        invalidateFilter();
    }
}

void TositeSelausProxyModel::suodataTositesarja(const QString &sarja)
{
    if( sarja != sarjaSuodatus_) {
        sarjaSuodatus_ = sarja;
        tyyppiSuodatus_ = -1;
        invalidateFilter();
    }
}

bool TositeSelausProxyModel::filterAcceptsRow(int source_row, const QModelIndex &source_parent) const
{
    if( tyyppiSuodatus_ > -1) {
        if(model_->tyyppi(source_row) != tyyppiSuodatus_)
            return false;
    }
    if( !sarjaSuodatus_.isEmpty() ) {
        if( model_->sarja(source_row) != sarjaSuodatus_)
            return false;
    }
    if( !etsiSuodatus_.isEmpty()) {
        if( !model_->etsiTeksti(source_row).contains(etsiSuodatus_, Qt::CaseInsensitive))
            return false;
    }
    return true;
}
