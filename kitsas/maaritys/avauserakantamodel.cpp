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
#include "avauserakantamodel.h"

AvausEraKantaModel::AvausEraKantaModel(QList<AvausEra> erat, QObject *parent) :
    QAbstractTableModel(parent), erat_(erat)
{
}

int AvausEraKantaModel::columnCount(const QModelIndex &parent) const
{
    if (parent.isValid())
        return 0;

    return 4;
}

QList<AvausEra> AvausEraKantaModel::erat() const
{
    QList<AvausEra> erat;
    for( const auto& era : qAsConst( erat_ ))
        if( era.saldo())
            erat.append(era);
    return erat;
}

qlonglong AvausEraKantaModel::summa() const
{
    qlonglong s = 0l;
    for( auto era : erat_)
        s += era.saldo();
    return s;
}

