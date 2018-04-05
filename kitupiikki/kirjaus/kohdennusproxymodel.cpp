/*
   Copyright (C) 2017 Arto Hyvättinen

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

#include "kohdennusproxymodel.h"
#include "db/kirjanpito.h"

KohdennusProxyModel::KohdennusProxyModel(QObject *parent, QDate paiva, int kohdennus)
    : QSortFilterProxyModel(parent),
      nykyinenPaiva(paiva),
      nykyinenKohdennus(kohdennus)
{
    setSourceModel( kp()->kohdennukset());
    sort(KohdennusModel::NIMI);
}

bool KohdennusProxyModel::filterAcceptsRow(int source_row, const QModelIndex & source_parent) const
{
    QModelIndex index = sourceModel()->index(source_row, 0, source_parent);

    if( index.data(KohdennusModel::IdRooli) == nykyinenKohdennus)
        return true;    // Valittu kohdennus on aina valittavissa ;)

    QDate alkaa = index.data(KohdennusModel::AlkaaRooli).toDate();
    QDate paattyy = index.data(KohdennusModel::PaattyyRooli).toDate();

    // Ei näytetä, jos ennen alkupäivää taikka loppupäivän jälkeen!
    if( alkaa.isValid() && nykyinenPaiva < alkaa)
        return false;
    if( paattyy.isValid() && nykyinenPaiva > paattyy)
        return false;
    if( index.data(KohdennusModel::TyyppiRooli).toInt() == Kohdennus::MERKKAUS)
        return false;   // Tageja ei näytetä kohdennusvalikoissa


    return true;
}
