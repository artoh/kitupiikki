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
#include "sqlitekysely.h"
#include "sqliteyhteys.h"

#include <QSqlQuery>

#include <QDebug>
#include <QDateTime>
#include <QMessageBox>
#include <QSqlError>

SQLiteKysely::SQLiteKysely(SQLiteYhteys *parent, KpKysely::Metodi metodi, QString polku)
    : KpKysely (parent, metodi, polku)
{

}

void SQLiteKysely::kysy(const QVariant &data)
{
    if( metodi() == GET && polku() == "aloita")
        alustusKysely();
    if( metodi() == PATCH && polku() == "asetukset")
        teeAsetus(data.toMap());
}

QSqlDatabase SQLiteKysely::tietokanta()
{
    return static_cast<SQLiteYhteys*>( parent() )->tietokanta();
}

void SQLiteKysely::alustusKysely()
{
    vastaus_.insert("asetukset", asetukset());

    vastaa();
}

QVariantList SQLiteKysely::asetukset()
{
    QVariantList lista;

    QSqlQuery query( tietokanta() );
    query.exec("SELECT avain, arvo, muokattu FROM asetus");
    while( query.next())
    {
        QVariantMap item;
        item.insert("avain", query.value("avain"));
        item.insert("arvo", query.value("arvo"));
        item.insert("muokattu", query.value("muokattu"));
        lista.append(item);
    }

    qDebug() << lista;

    return lista;
}

void SQLiteKysely::teeAsetus(const QVariantMap& params)
{
    QSqlQuery query(tietokanta());

    if( params.value("arvo").isNull())
    {
        query.prepare("DELETE FROM asetus WHERE avain=:avain");
    }
    else
    {
        query.prepare("REPLACE INTO asetus(avain, arvo, muokattu) VALUES(:avain,:arvo,:muokattu)");
        query.bindValue(":arvo", params.value("arvo"));
        query.bindValue(":muokattu", QDateTime::currentDateTime() );
    }

    query.bindValue(":avain", params.value("avain"));

    if( query.exec())
        vastaa();
    else
        QMessageBox::critical(nullptr, tr("Tietokantavirhe"),
                              tr("Asetuksen tallentaminen epäonnistui seuraavan virheen takia:%1")
                              .arg(tietokanta().lastError().text()));

}
