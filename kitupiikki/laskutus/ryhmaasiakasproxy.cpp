/*
   Copyright (C) 2018 Arto Hyvättinen

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
#include "ryhmaasiakasproxy.h"
#include "laskuryhmamodel.h"
#include "asiakkaatmodel.h"

#include <QDebug>

RyhmaAsiakasProxy::RyhmaAsiakasProxy(QObject *parent)
    : QSortFilterProxyModel (parent)
{

}

void RyhmaAsiakasProxy::asetaRyhmaModel(LaskuRyhmaModel *model)
{
    ryhmamodel_ = model;
    connect( ryhmamodel_, &LaskuRyhmaModel::rowsInserted, this, &RyhmaAsiakasProxy::invalidateFilter);
    connect( ryhmamodel_, &LaskuRyhmaModel::rowsRemoved, this, &RyhmaAsiakasProxy::invalidateFilter);
}

bool RyhmaAsiakasProxy::filterAcceptsRow(int source_row, const QModelIndex &source_parent) const
{
    QModelIndex index = sourceModel()->index(source_row, 0, source_parent);

    if( ryhmamodel_->onkoNimella( index.data(AsiakkaatModel::NimiRooli).toString()  ) )
        return false;
    return QSortFilterProxyModel::filterAcceptsRow(source_row, source_parent);
}
