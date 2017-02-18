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

#include "tositelaji.h"

#include <QSqlQuery>

Tositelaji::Tositelaji() :
    id_(0), muokattu_(false)
{

}

Tositelaji::Tositelaji(int id, QString tunnus, QString nimi, QByteArray json) :
    id_(id), tunnus_(tunnus), nimi_(nimi), muokattu_(false)
{
    if( !json.isEmpty())
        json_.fromJson(json);
}

void Tositelaji::asetaId(int id)
{
    id_ = id;
}

void Tositelaji::asetaTunnus(const QString &tunnus)
{
    tunnus_ = tunnus;
    muokattu_ = true;
}

void Tositelaji::asetaNimi(const QString &nimi)
{
    nimi_ = nimi;
    muokattu_ = true;
}

void Tositelaji::nollaaMuokattu()
{
    muokattu_ = false;
}

int Tositelaji::montakoTositetta() const
{
    if( id() == 0)
        return 0;

    QSqlQuery kysely( QString("SELECT COUNT(id) FROM tosite WHERE laji=%1").arg( id()) );
    if( kysely.next())
        return kysely.value(0).toInt();

    return 0;
}

