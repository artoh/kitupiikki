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
#ifndef SQLITEALUSTAJA_H
#define SQLITEALUSTAJA_H

#include <QObject>
#include <QSqlDatabase>
#include <QVariantMap>

class QProgressDialog;

class SqliteAlustaja : public QObject
{
    Q_OBJECT
public:
    static bool luoKirjanpito(const QString& polku, const QVariantMap& initials);

protected:
    SqliteAlustaja();
    ~SqliteAlustaja() override;

    bool alustaTietokanta(const QString& polku);
    bool lopputoimet();

    QSqlDatabase db;
    QProgressDialog *progress = nullptr;

};

#endif // SQLITEALUSTAJA_H
