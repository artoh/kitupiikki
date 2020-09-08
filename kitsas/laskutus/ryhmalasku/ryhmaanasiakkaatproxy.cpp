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
#include "ryhmaanasiakkaatproxy.h"

#include "laskutettavatmodel.h"
#include "../asiakkaatmodel.h"

RyhmaanAsiakkaatProxy::RyhmaanAsiakkaatProxy(QObject *parent)
    : QSortFilterProxyModel(parent)
{

}

void RyhmaanAsiakkaatProxy::asetaLaskutettavatModel(LaskutettavatModel *model)
{
    laskutettavat_ = model;
}

bool RyhmaanAsiakkaatProxy::filterAcceptsRow(int source_row, const QModelIndex &source_parent) const
{
    if( !laskutettavat_)
        return true;
    QModelIndex index = sourceModel()->index(source_row,0,source_parent);

    int kumppaniId = index.data(AsiakkaatModel::IdRooli).toInt();

    return ( !kumppaniId || !laskutettavat_->onkoKumppania(kumppaniId)) &&
            QSortFilterProxyModel::filterAcceptsRow(source_row, source_parent);
}
