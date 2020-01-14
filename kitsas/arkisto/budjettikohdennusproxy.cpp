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
#include "budjettikohdennusproxy.h"
#include "db/kohdennusmodel.h"
#include "db/kirjanpito.h"

BudjettiKohdennusProxy::BudjettiKohdennusProxy(QObject *parent) :
    QSortFilterProxyModel (parent)
{
    setSourceModel( kp()->kohdennukset() );
    sort( KohdennusModel::NIMI);
}

void BudjettiKohdennusProxy::asetaKausi(const QDate &alkaa, const QDate &paattyy)
{
    alkaa_ = alkaa;
    paattyy_ = paattyy;
    invalidate();
}

bool BudjettiKohdennusProxy::filterAcceptsRow(int source_row, const QModelIndex &source_parent) const
{
    QModelIndex index = sourceModel()->index(source_row, 0, source_parent);


    QDate alkaa = index.data(KohdennusModel::AlkaaRooli).toDate();
    QDate paattyy = index.data(KohdennusModel::PaattyyRooli).toDate();

    // Ei näytetä, jos ennen alkupäivää taikka loppupäivän jälkeen!
    if( alkaa.isValid() && alkaa > paattyy_)
        return false;
    if( paattyy.isValid() && paattyy < alkaa_)
        return false;

    int tyyppi = index.data(KohdennusModel::TyyppiRooli).toInt();

    if( tyyppi == Kohdennus::MERKKAUS)
        return false;

    return true;
}
