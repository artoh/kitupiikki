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

#include <QDebug>

JsonKentta::JsonKentta() : muokattu_(false)
{

}

JsonKentta::JsonKentta(const QByteArray &json)
{
    fromJson(json);
}

void JsonKentta::set(const QString &avain, const QString &arvo)
{
    if( arvo != map_.value(avain).toString())
    {
        map_[avain] = QVariant(arvo);
        muokattu_ = true;
    }
}

void JsonKentta::set(const QString &avain, const QDate &pvm)
{
    if( pvm != QDate::fromString(map_.value(avain).toString()), Qt::ISODate)
    {
        map_[avain] = QVariant(pvm.toString(Qt::ISODate));
        muokattu_ = true;
    }
}

void JsonKentta::set(const QString &avain, int arvo)
{
    if( map_.value(avain).toInt() != arvo )
    {
        map_[avain] = QVariant(arvo);
        muokattu_ = true;
    }
}

void JsonKentta::set(const QString &avain, qulonglong arvo)
{
    if( map_.value(avain).toULongLong() != arvo )
    {
        map_[avain] = QVariant(arvo);
        muokattu_ = true;
    }
}

void JsonKentta::unset(const QString &avain)
{
    if( map_.contains(avain))
    {
        map_.remove(avain);
        muokattu_ = true;
    }
}

void JsonKentta::setVar(const QString &avain, const QVariant &arvo)
{
    if( map_.value(avain) != arvo)
    {
        map_[avain] = arvo;
        muokattu_ = true;
    }
}

QString JsonKentta::str(const QString &avain)
{
    return map_.value(avain).toString();
}

QDate JsonKentta::date(const QString &avain)
{
    return QDate::fromString( map_.value(avain).toString() , Qt::ISODate);
}

int JsonKentta::luku(const QString &avain, int oletus)
{
    return map_.value(avain, QString::number(oletus) ).toInt();
}

qulonglong JsonKentta::isoluku(const QString &avain)
{
    return map_.value(avain).toULongLong();
}

QVariant JsonKentta::variant(const QString &avain)
{
    return map_.value(avain);
}

QByteArray JsonKentta::toJson()
{
    QJsonDocument doc( QJsonObject::fromVariantMap( map_ ));
    return doc.toJson( QJsonDocument::Compact);
}

QVariant JsonKentta::toSqlJson()
{
    muokattu_ = false;
    if( map_.count())
        return QVariant( toJson() );
    else
        return QVariant();
}

void JsonKentta::fromJson(const QByteArray &json)
{
    QJsonDocument doc = QJsonDocument::fromJson( json );
    map_ = doc.object().toVariantMap();
    muokattu_ = false;
}
