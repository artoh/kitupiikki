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
#include "sqliteroute.h"

SQLiteRoute::SQLiteRoute(SQLiteModel *model, const QString &polku)
    : model_(model), polku_(polku)
{

}

SQLiteRoute::~SQLiteRoute()
{

}

QVariant SQLiteRoute::route(SQLiteKysely *kysely, const QVariant &data)
{
    QString loppu = kysely->polku().mid( polku().length() );

    switch (kysely->metodi()) {
    case KpKysely::GET:
        return get(loppu);
    case KpKysely::POST:
        return post(loppu, data);
    case KpKysely::PUT:
        return put(loppu, data);
    case KpKysely::PATCH:
        return patch(loppu, data);
    case KpKysely::DELETE:
        return doDelete(loppu);
    }
}

QVariant SQLiteRoute::byteArray(SQLiteKysely * /*reititettavaKysely*/, const QByteArray & /*ba*/, const QVariantMap& /*meta*/)
{
    return QVariant();
}

QVariant SQLiteRoute::get(const QString & /*polku*/)
{
    return QVariant();
}

QVariant SQLiteRoute::put(const QString & /*polku*/, const QVariant &/*data*/)
{
    return QVariant();
}

QVariant SQLiteRoute::post(const QString &/*polku*/, const QVariant &/*data*/)
{
    return QVariant();
    }

    QVariant SQLiteRoute::patch(const QString &/*polku*/, const QVariant &/*data*/)
    {

    }

QVariant SQLiteRoute::doDelete(const QString &/*polku*/)
{
    return QVariant();
}

QSqlDatabase SQLiteRoute::db()
{
    return model_->tietokanta();
}
