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
#ifndef RYHMATROUTE_H
#define RYHMATROUTE_H

#include "../sqliteroute.h"

class RyhmatRoute : public SQLiteRoute
{
public:
    RyhmatRoute(SQLiteModel* model);

    QVariant get(const QString &polku, const QUrlQuery &urlquery = QUrlQuery()) override;
    QVariant post(const QString &polku, const QVariant &data) override;
    QVariant put(const QString &polku, const QVariant &data) override;
    QVariant doDelete(const QString &polku) override;

};

#endif // RYHMATROUTE_H
