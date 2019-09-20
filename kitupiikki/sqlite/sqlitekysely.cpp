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
#include "sqlitemodel.h"

#include <QSqlQuery>

#include <QDebug>
#include <QDateTime>
#include <QMessageBox>
#include <QSqlError>
#include <QJsonDocument>
#include <QDebug>

SQLiteKysely::SQLiteKysely(SQLiteModel *parent, KpKysely::Metodi metodi, QString polku)
    : KpKysely (parent, metodi, polku)
{

}

void SQLiteKysely::kysy(const QVariant &data)
{
    try {
        SQLiteModel* model = qobject_cast<SQLiteModel*>( parent() );
        model->reitita(this, data);
    } catch ( SQLiteVirhe &e ) {
        emit virhe( e.koodi(), e.selitys() );
        qDebug() << "[" << e.koodi() << " " << polku() << "] " << e.selitys();
    }
}

void SQLiteKysely::lahetaTiedosto(const QByteArray &ba, const QMap<QString, QString> &meta)
{
    try {
        SQLiteModel* model = qobject_cast<SQLiteModel*>( parent() );
        QMap<QString,QString> omameta(meta);
        if( !omameta.contains("Content-type")) {
            QString arvattu = tiedostotyyppi(ba);
            if( !arvattu.isNull())
                omameta.insert("Content-type", arvattu);
        }
        model->reitita(this, ba, omameta);
    } catch ( SQLiteVirhe &e ) {
        emit virhe( e.koodi(), e.selitys() );
        qDebug() << "[" << e.koodi() << " " << polku() << "] " << e.selitys();
    }
}


void SQLiteKysely::vastaa(const QVariant &tulos)
{
    vastaus_ = tulos;
    emit vastaus(&vastaus_);
}

SQLiteVirhe::SQLiteVirhe(const QString &selitys, int virhekoodi) :
    selitys_(selitys), koodi_(virhekoodi)
{

}

SQLiteVirhe::SQLiteVirhe(const QSqlQuery &kysely)
{
    selitys_ = kysely.lastError().text() + " : " + kysely.lastQuery();
    koodi_ = 500;
}

QString SQLiteVirhe::selitys() const
{
    return selitys_;
}

int SQLiteVirhe::koodi() const
{
    return koodi_;
}


