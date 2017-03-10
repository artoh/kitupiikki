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
#include <QVariant>

#include "tilikausi.h"

Tilikausi::Tilikausi()
{

}

Tilikausi::Tilikausi(QDate tkalkaa, QDate tkpaattyy, QByteArray json) :
    alkaa_(tkalkaa),
    paattyy_(tkpaattyy),
    json_(json)
{

}

QString Tilikausi::kausivaliTekstina() const
{
    return QString("%1 - %2")
            .arg( alkaa().toString(Qt::SystemLocaleShortDate))
            .arg( paattyy().toString(Qt::SystemLocaleShortDate));
}

int Tilikausi::tulos() const
{
    QSqlQuery kysely(  QString("SELECT SUM(kreditsnt), SUM(debetsnt) "
                               "FROM vienti, tili WHERE "
                               "pvm BETWEEN \"%1\" AND \"%2\" "
                               "AND vienti.tili=tili.id AND "
                               "tili.ysiluku > 300000000")
                       .arg(alkaa().toString(Qt::ISODate))
                       .arg(paattyy().toString(Qt::ISODate)));
    if( kysely.next())
        return kysely.value(0).toInt() - kysely.value(1).toInt();
    else
        return 0;
}

QString Tilikausi::arkistoHakemistoNimi() const
{
    if( alkaa().month() == 1 && alkaa().day() == 1)
        return alkaa().toString("yyyy");
    else if( alkaa().day() == 1)
        return alkaa().toString("yyyy-MM");
    else
        return alkaa().toString("yyyy-MM-dd");
}
