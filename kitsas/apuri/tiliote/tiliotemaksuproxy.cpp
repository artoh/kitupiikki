/*
   Copyright (C) 2026 Kitsas Oy

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
#include "tiliotemaksuproxy.h"
#include "model/laskutaulumodel.h"

#include <QRegularExpression>

TilioteMaksuProxy::TilioteMaksuProxy(QObject *parent) :
    QSortFilterProxyModel(parent)
{
}

void TilioteMaksuProxy::suodata(const QString &teksti)
{
    QString siivottu = teksti;
    siivottu.remove(' ');

    if( siivottu.startsWith("RF", Qt::CaseInsensitive)) {
        // RF-viitteestä poistetaan RF-tunnus ja tarkistenumerot,
        // jäljelle jäävä osa on kansallinen viitenumero
        numeroHaku_ = siivottu.mid(4);
        setFilterFixedString(QString());
    } else if( !siivottu.isEmpty() && siivottu.contains(QRegularExpression("^\\d+$"))) {
        numeroHaku_ = siivottu;
        setFilterFixedString(QString());
    } else {
        numeroHaku_.clear();
        setFilterKeyColumn( LaskuTauluModel::ASIAKASTOIMITTAJA );
        setFilterCaseSensitivity( Qt::CaseInsensitive );
        setFilterFixedString( teksti );
    }
    invalidateFilter();
}

bool TilioteMaksuProxy::filterAcceptsRow(int source_row, const QModelIndex &source_parent) const
{
    if( numeroHaku_.isEmpty())
        return QSortFilterProxyModel::filterAcceptsRow(source_row, source_parent);

    const QModelIndex index = sourceModel()->index(source_row, LaskuTauluModel::NUMERO, source_parent);
    const QString numero = index.data(Qt::DisplayRole).toString();
    QString viite = index.data(LaskuTauluModel::ViiteRooli).toString();
    viite.remove(' ');

    return numero.contains(numeroHaku_) || viite.contains(numeroHaku_);
}
