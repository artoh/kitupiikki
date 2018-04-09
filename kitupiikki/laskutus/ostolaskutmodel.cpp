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

#include <QSqlQuery>

#include "db/eranvalintamodel.h"
#include "ostolaskutmodel.h"

OstolaskutModel::OstolaskutModel(QObject *parent)
    : LaskutModel(parent)
{

}

QVariant OstolaskutModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if( role == Qt::DisplayRole && orientation == Qt::Horizontal && section == ASIAKAS)
        return tr("Laskuttaja");
    return LaskutModel::headerData(section, orientation, role);
}

void OstolaskutModel::lataaAvoimet()
{
    QString kysely = QString("SELECT id, pvm, tili, debetsnt, kreditsnt, eraid, viite, erapvm, json, tosite, asiakas, laskupvm, kohdennus FROM vienti "
                     "WHERE viite IS NOT NULL AND iban IS NOT NULL");

    beginResetModel();
    laskut.clear();
    QSqlQuery query( kysely );

    while( query.next())
    {
        TaseEra era( query.value("eraid").toInt());

        JsonKentta json( query.value("json").toByteArray() );

        if( !era.saldoSnt )
            continue;

        // Tämä lasku kelpaa ;)
        AvoinLasku lasku;
        lasku.viite = query.value("viite").toString();
        lasku.pvm = query.value("laskupvm").toDate();
        lasku.erapvm = query.value("erapvm").toDate();
        lasku.eraId = query.value("eraid").toInt();
        lasku.summaSnt = query.value("kreditSnt").toInt() -  query.value("debetSnt").toInt();
        lasku.avoinSnt = 0 - era.saldoSnt;
        lasku.asiakas = query.value("asiakas").toString();
        lasku.tosite = query.value("tosite").toInt();
        lasku.kirjausperuste =  json.luku("Kirjausperuste");
        lasku.tiliid = query.value("tili").toInt();
        lasku.json = json;
        lasku.kohdennusId = query.value("kohdennus").toInt();
        laskut.append(lasku);
    }
    endResetModel();
}


