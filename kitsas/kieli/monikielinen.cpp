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
#include "monikielinen.h"

#include <QJsonDocument>

Monikielinen::Monikielinen()
{

}

Monikielinen::Monikielinen(const QVariant &var)
{
    Monikielinen::aseta(var);
}

Monikielinen::Monikielinen(const QString &str)
{
    QJsonDocument doc = QJsonDocument::fromJson( str.toUtf8() );
    Monikielinen::aseta( doc.toVariant());
}


void Monikielinen::aseta(const QVariant &var)
{
    tekstit_.clear();
    if( !var.toMap().isEmpty() ) {
        QMapIterator<QString,QVariant> iter(var.toMap());
        while( iter.hasNext()) {
            iter.next();
            tekstit_.insert( iter.key(), iter.value().toString());
        }
    } else if(!var.toString().isEmpty()) {
        tekstit_.insert("fi", var.toString());
    }
}

void Monikielinen::aseta(const QString &nimi, const QString &kieli)
{
    if( nimi.isEmpty())
        tekstit_.remove(kieli);
    else
        tekstit_.insert(kieli, nimi);
}

QString Monikielinen::teksti(const QString &kieli) const
{
    if( kieli.isEmpty()) {
        if( tekstit_.contains(oletuskieli__)) {
            return tekstit_.value(oletuskieli__);
        }
    } else if( tekstit_.contains(kieli)) {
        return tekstit_.value(kieli);
    }
    if( tekstit_.contains("fi"))
        return tekstit_.value("fi");
    if( !tekstit_.isEmpty())
        return tekstit_.first();
    return QString();
}

QString Monikielinen::kaannos(const QString &kieli) const
{
    return tekstit_.value(kieli);
}

QVariantMap Monikielinen::map() const
{
    QVariantMap out;
    QMapIterator<QString,QString> iter(tekstit_);
    while( iter.hasNext()) {
        iter.next();
        out.insert( iter.key(), iter.value());
    }
    return out;
}

void Monikielinen::asetaOletuskieli(const QString &kieli)
{
    oletuskieli__ = kieli;
}

QString Monikielinen::oletuskieli__ = QString();
