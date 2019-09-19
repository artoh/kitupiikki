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
#ifndef SQLITEROUTE_H
#define SQLITEROUTE_H

#include "sqlitemodel.h"
#include "sqlitekysely.h"

#include <QSqlDatabase>
#include <QSqlQuery>

#include <exception>

class SQLiteRoute
{
public:
    SQLiteRoute(SQLiteModel *model, const QString& polku);
    virtual ~SQLiteRoute();

    QVariant route(SQLiteKysely* kysely, const QVariant& data);
    virtual QVariant byteArray(SQLiteKysely*, const QByteArray &, const QVariantMap &meta);

    QString polku() const { return polku_;}

protected:

    virtual QVariant get(const QString& polku, const QUrlQuery& urlquery = QUrlQuery());
    virtual QVariant put(const QString& polku, const QVariant& data);
    virtual QVariant post(const QString& polku, const QVariant& data);
    virtual QVariant patch(const QString& polku, const QVariant& data);
    virtual QVariant doDelete(const QString& polku);

    QVariantList resultList(QSqlQuery& kysely);
    QVariantMap resultMap(QSqlQuery& kysely);
    QByteArray mapToJson(const QVariantMap& map);

protected:
    QSqlDatabase db();
    SQLiteModel *model_;
    QString polku_;
};

#endif // SQLITEROUTE_H
