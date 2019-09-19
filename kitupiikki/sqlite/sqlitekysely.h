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
#include <exception>


class SQLiteVirhe : public std::exception
{
public:
    SQLiteVirhe(const QString& selitys = QString(), int virhekoodi = 400);
    QString selitys() const;
    int koodi() const;
private:
    QString selitys_;
    int koodi_;
};

class SQLiteModel;

class SQLiteKysely : public KpKysely
{
    Q_OBJECT
public:
    SQLiteKysely(SQLiteModel* parent, Metodi metodi=GET, QString polku = QString());
    void vastaa(const QVariant& tulos);

public slots:
    void kysy(const QVariant& data = QVariant()) override;
    virtual void lahetaTiedosto(const QByteArray& ba, const QString& tiedostonimi) override;   

};

#endif // SQLITEKYSELY_H
