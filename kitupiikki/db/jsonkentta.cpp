/*
   Copyright (C) 2017 Arto Hyvättinen

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

#include "jsonkentta.h"
#include <QString>

#include <QJsonDocument>
#include <QJsonObject>

JsonKentta::JsonKentta()
{

}

void JsonKentta::set(const QString &avain, const QString &arvo)
{
    map_[avain] = QVariant(arvo);
}

void JsonKentta::set(const QString &avain, const QDate &pvm)
{
    map_[avain] = QVariant(pvm.toString(Qt::ISODate));
}

void JsonKentta::set(const QString &avain, int arvo)
{
    map_[avain] = QVariant(arvo);
}

void JsonKentta::unset(const QString &avain)
{
    map_.remove(avain);
}

QString JsonKentta::str(const QString &avain)
{
    return map_.value(avain).toString();
}

QDate JsonKentta::date(const QString &avain)
{
    return QDate::fromString( map_.value(avain).toString() , Qt::ISODate);
}

int JsonKentta::luku(const QString &avain)
{
    return map_.value(avain).toInt();
}

QByteArray JsonKentta::toJson()
{
    QJsonDocument doc( QJsonObject::fromVariantMap( map_ ));
    return doc.toJson( QJsonDocument::Compact);
}

QVariant JsonKentta::toSqlJson()
{
    if( map_.count())
        return QVariant( toJson() );
    else
        return QVariant();


}

void JsonKentta::fromJson(const QByteArray &json)
{
    QJsonDocument doc = QJsonDocument::fromJson( json );
    map_ = doc.object().toVariantMap();
}
