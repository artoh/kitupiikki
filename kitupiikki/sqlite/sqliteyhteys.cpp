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
#include "db/kirjanpito.h"

#include <QDebug>
#include <QSqlError>
#include <QSettings>
#include <QMessageBox>

SQLiteYhteys::SQLiteYhteys(QObject *parent, const QString &tietokanta) :
    KpYhteys (parent), tiedostoPolku_(tietokanta)
{
    tietokanta_ = QSqlDatabase::addDatabase("QSQLITE", "TOINEN");
    tietokanta_.setDatabaseName( tietokanta );
}

bool SQLiteYhteys::alustaYhteys(bool ilmoitaVirheestaAvattauessa)
{
    if( !tietokanta_.open())
    {
        if( ilmoitaVirheestaAvattauessa ) {
            QMessageBox::critical(nullptr, tr("Tietokannan avaaminen epäonnistui"),
                                  tr("Tietokannan %1 avaaminen epäonnistui tietokantavirheen %2 takia")
                                  .arg( tiedostopolku() ).arg( tietokanta().lastError().text() ) );
        }
        qDebug() << "SQLiteYhteys: Tietokannan avaaminen epäonnistui : " << tietokanta_.lastError().text();
        return false;
    }

    SQLiteKysely *alustusKysely = kysely("/init");
    connect( alustusKysely, &SQLiteKysely::vastaus, this, &SQLiteYhteys::initSaapui );
    alustusKysely->kysy();    
    return true;
}

void SQLiteYhteys::initSaapui(QVariantMap * /* reply */, int tila)
{
    emit yhteysAvattu( tila == SQLiteKysely::OK );
    if( tila == SQLiteKysely::OK)
        kp()->settings()->setValue("Viimeisin", tiedostopolku());
    sender()->deleteLater();
}


SQLiteKysely *SQLiteYhteys::kysely(const QString& polku, KpKysely::Metodi metodi)
{
    return new SQLiteKysely( this, metodi, polku);
}
