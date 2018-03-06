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

#include <QSqlQuery>

#include "alvilmoitustenmodel.h"
#include "db/kirjanpito.h"
#include "db/jsonkentta.h"

AlvIlmoitustenModel::AlvIlmoitustenModel(QObject *parent)
    : QAbstractTableModel(parent)
{
    lataa();
}

int AlvIlmoitustenModel::rowCount(const QModelIndex &/*parent*/) const
{
    return tiedot_.count();
}

int AlvIlmoitustenModel::columnCount(const QModelIndex &/*parent*/) const
{
    return 4;
}

QVariant AlvIlmoitustenModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if( role == Qt::TextAlignmentRole)
    {
            return QVariant( Qt::AlignCenter | Qt::AlignVCenter);
    }
    else if( orientation == Qt::Horizontal && role == Qt::DisplayRole)
    {
        switch (section) {
        case ALKAA:
            return tr("Alkaa");
        case PAATTYY:
            return tr("Päättyy");
        case ERAPVM:
            return tr("Eräpäivä");
        case VEROSNT:
            return tr("Maksettava vero");
        }
    }
    return QVariant();
}

QVariant AlvIlmoitustenModel::data(const QModelIndex &index, int role) const
{
    if( !index.isValid())
    {
        return QVariant();
    }

    AlvIlmoitusTieto tieto = tiedot_.value(index.row());

    if( role == Qt::DisplayRole )
    {
        switch (index.column()) {
        case ALKAA:
            return tieto.alkuPvm;
        case PAATTYY:
            return tieto.loppuPvm;
        case ERAPVM:
            return tieto.loppuPvm.addDays(1).addMonths(1).addDays(11);
        case VEROSNT:
            return QString("%L1 €").arg( tieto.maksettavaVeroSnt / 100.0 , 0,'f',2);
        }
    }
    else if( role == Qt::TextAlignmentRole)
    {
        if( index.column()==VEROSNT)
            return QVariant(Qt::AlignRight | Qt::AlignVCenter);
        else
            return QVariant( Qt::AlignLeft | Qt::AlignVCenter);

    }
    else if( role == TositeIdRooli )
        return tieto.tositeId;

    return QVariant();

}

void AlvIlmoitustenModel::lataa()
{
    beginResetModel();
    tiedot_.clear();

    QSqlQuery kysely("select id,json from tosite where laji=0 order by pvm desc");
    while( kysely.next())
    {
        JsonKentta json;
        json.fromJson( kysely.value("json").toByteArray());
        if( json.date("AlvTilitysAlkaa").isValid())
        {
            AlvIlmoitusTieto ilmoitus;
            ilmoitus.alkuPvm = json.date("AlvTilitysAlkaa");
            ilmoitus.loppuPvm = json.date("AlvTilitysPaattyy");
            ilmoitus.maksettavaVeroSnt = json.luku("MaksettavaAlv");
            ilmoitus.tositeId = kysely.value("id").toInt();
            tiedot_.append(ilmoitus);
        }
    }
    endResetModel();
}
