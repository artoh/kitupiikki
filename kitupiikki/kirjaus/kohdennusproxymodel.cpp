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

#include <QMenu>

#include "kohdennusproxymodel.h"
#include "db/kirjanpito.h"

KohdennusProxyModel::KohdennusProxyModel(QObject *parent, QDate paiva, int kohdennus, Naytettavat naytetaan)
    : QSortFilterProxyModel(parent),
      nykyinenPaiva(paiva),
      nykyinenKohdennus(kohdennus),
      naytettavat(naytetaan)
{
    setSourceModel( kp()->kohdennukset());
    sort(KohdennusModel::NIMI);
}

void KohdennusProxyModel::asetaVali(const QDate &alkupvm, const QDate &loppupvm)
{
    nykyinenPaiva = alkupvm;
    nykyinenPaiva = loppupvm;
    invalidate();
}

bool KohdennusProxyModel::filterAcceptsRow(int source_row, const QModelIndex & source_parent) const
{
    QModelIndex index = sourceModel()->index(source_row, 0, source_parent);

    if( index.data(KohdennusModel::IdRooli) == nykyinenKohdennus)
        return true;    // Valittu kohdennus on aina valittavissa ;)

    QDate alkaa = index.data(KohdennusModel::AlkaaRooli).toDate();
    QDate paattyy = index.data(KohdennusModel::PaattyyRooli).toDate();

    if( paattyy.isValid()) {
        if( alkaa.isValid() && nykyinenPaattyy < alkaa )
            return false;
        if( paattyy.isValid() && nykyinenPaiva > paattyy)
            return false;
    } else {
        // Ei näytetä, jos ennen alkupäivää taikka loppupäivän jälkeen!
        if( alkaa.isValid() && nykyinenPaiva < alkaa)
            return false;
        if( paattyy.isValid() && nykyinenPaiva > paattyy)
            return false;
    }

    int tyyppi = index.data(KohdennusModel::TyyppiRooli).toInt();

    if( naytettavat == KOHDENNUKSET_PROJEKTIT && tyyppi == Kohdennus::MERKKAUS )
        return false;
    else if( naytettavat == MERKKKAUKSET && tyyppi != Kohdennus::MERKKAUS)
        return false;



    return true;
}
