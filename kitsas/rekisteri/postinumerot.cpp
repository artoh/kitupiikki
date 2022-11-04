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
#include "postinumerot.h"

#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>

#include <QDebug>

Postinumerot::Postinumerot()
{
}

QString Postinumerot::toimipaikka(const QString &postinumero)
{
    if( numerot__.toimipaikat_.isEmpty())
        numerot__.alusta();

    return numerot__.toimipaikat_.value(postinumero);
}

void Postinumerot::alusta()
{
    QFile in(":/lasku/postcode.json");
    in.open(QFile::ReadOnly | QFile::Text);

    QJsonObject obj = QJsonDocument::fromJson( in.readAll() ).object();
    for( auto i = obj.constBegin(); i != obj.constEnd(); ++i  )
        toimipaikat_.insert( i.key(), i.value().toString() );
}

Postinumerot Postinumerot::numerot__;
