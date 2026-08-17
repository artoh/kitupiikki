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
#include "sqlitealustaja.h"
#include "sql/sqlalustaja.h"

#include <QSqlQuery>
#include <QMessageBox>
#include <QApplication>
#include <QSqlError>
#include <QProgressDialog>

bool SqliteAlustaja::luoKirjanpito(const QString &polku, const QVariantMap &initials)
{
    SqliteAlustaja alustaja;

    QVariantMap initMap = initials.value("init").toMap();

    bool onnistui =  alustaja.alustaTietokanta(polku) &&
            SqlAlustaja::kirjoitaInit(alustaja.db, initMap, alustaja.progress) &&
            alustaja.lopputoimet() &&
            !alustaja.progress->wasCanceled();

    alustaja.progress->close();

    return onnistui;
}

SqliteAlustaja::SqliteAlustaja() :
    QObject(nullptr)
{
    progress = new QProgressDialog(tr("Alustetaan kirjanpitoa..."), tr("Peruuta"),0,10);
    progress->setMinimumDuration(500);

    db = QSqlDatabase::addDatabase("QSQLITE","uusi");
}

bool SqliteAlustaja::alustaTietokanta(const QString &polku)
{

    progress->setValue(1);
    qApp->processEvents();

    db.setDatabaseName(polku);
    if( !db.open() ){
        QMessageBox::critical(nullptr, tr("Kirjanpidon %1 luominen epäonnistui").arg(polku), tr("Tietokannan luominen epäonnistui seuraavan virheen takia: %1").arg( db.lastError().text() ));
        return false;
    }
    progress->setValue(2);

    // Kirjanpidon luomisen ajaksi synkronointi pois käytöstä
    db.exec("PRAGMA SYNCHRONOUS = OFF");

    if( !SqlAlustaja::suoritaSqlResurssi(db, QStringLiteral(":/sqlite/luo.sql")) )
        return false;

    progress->setValue(6);
    return true;
}

bool SqliteAlustaja::lopputoimet()
{
    db.exec("PRAGMA SYNCHRONOUS = NORMAL");
    db.exec("PRAGMA JOURNAL_MODE = DELETE");
#ifndef KITSAS_DEVEL
    db.exec("PRAGMA LOCKING_MODE = EXCLUSIVE");
#endif
    progress->setValue(10);
    return true;
}
