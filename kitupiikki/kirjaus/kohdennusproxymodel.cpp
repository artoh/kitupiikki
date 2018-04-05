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

KohdennusProxyModel::KohdennusProxyModel(QObject *parent, QDate paiva, int kohdennus)
    : QSortFilterProxyModel(parent),
      nykyinenPaiva(paiva),
      nykyinenKohdennus(kohdennus)
{
    setSourceModel( kp()->kohdennukset());
    sort(KohdennusModel::NIMI);
}

QVariantList KohdennusProxyModel::tagiValikko(const QDate& pvm, QVariantList valitut)
{
    // Valikko tägien valitsemiseen
    QMenu tagvalikko;

    KohdennusProxyModel proxy(0, pvm, KohdennusProxyModel::MERKKAUKSET);
    for(int i=0; i < proxy.rowCount(QModelIndex()); i++)
    {
        QModelIndex pInd = proxy.index(i, 0);
        QAction *aktio = tagvalikko.addAction( QIcon(":/pic/tag.png"), pInd.data(KohdennusModel::NimiRooli).toString());
        aktio->setData( pInd.data(KohdennusModel::IdRooli) );
        aktio->setCheckable(true);
        if( valitut.contains( QVariant( pInd.data(KohdennusModel::IdRooli) ) ) )
            aktio->setChecked(true);
    }

    tagvalikko.exec( QCursor::pos() );

    // Uusi valinta, jossa valitut tagit
    QVariantList uusivalinta;
    for( QAction* aktio : tagvalikko.actions() )
    {
        if( aktio->isChecked())
            uusivalinta.append( aktio->data());
    }
    return uusivalinta;
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

    if( (nykyinenKohdennus >= -1 && index.data(KohdennusModel::TyyppiRooli).toInt() == Kohdennus::MERKKAUS) ||
        (nykyinenKohdennus == -10 && index.data(KohdennusModel::TyyppiRooli).toInt() != Kohdennus::MERKKAUS) )
        return false;   // Tageja ei näytetä kohdennusvalikoissa eikä kohdennuksia tagivalikossa


    return true;
}
