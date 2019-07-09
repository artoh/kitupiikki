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
#ifndef SQLITEKYSELY_H
#define SQLITEKYSELY_H

#include "db/kpkysely.h"

#include <QSqlDatabase>

class SQLiteModel;

class SQLiteKysely : public KpKysely
{
    Q_OBJECT
public:
    SQLiteKysely(SQLiteModel* parent, Metodi metodi=GET, QString polku = QString());

public slots:
    void kysy(const QVariant& data = QVariant()) override;

protected:
    QSqlDatabase tietokanta();

    void alustusKysely();
    QVariantList asetukset();
    QVariantList tilit();
    QVariantList kohdennukset();
    QVariantList tositelajit();
    QVariantList tilikaudet();

    void teeAsetus(const QVariantMap &params);
    void lataaLiite();

    QVariantMap tosite(int id);
    void tositelista();

    void vientilista();

    QStringList sanat_;

    void vastaa();
};

#endif // SQLITEKYSELY_H
