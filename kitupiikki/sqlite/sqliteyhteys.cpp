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
#include "sqliteyhteys.h"

#include <QDebug>
#include <QSqlError>

SQLiteYhteys::SQLiteYhteys(Kirjanpito *parent, const QUrl &url) :
    KpYhteys (parent, url)
{
    tietokanta_ = QSqlDatabase::addDatabase("QSQLITE", "TOINEN");
}

bool SQLiteYhteys::avaaYhteys()
{
    tietokanta_.setDatabaseName( url().toLocalFile() );

    qDebug() << "SQLiteYhteys: Avataan tietokanta " << url();

    if( !tietokanta_.open())
    {
        qDebug() << "SQLiteYhteys: Tietokannan avaaminen epäonnistui : " << tietokanta_.lastError().text();
        return false;
    }


    return true;
}

SQLiteKysely *SQLiteYhteys::kysely(const QString& polku, KpKysely::Metodi metodi)
{
    return new SQLiteKysely( this, metodi, polku);
}
