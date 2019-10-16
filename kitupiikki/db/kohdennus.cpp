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

#include <QSqlQuery>
#include <QVariant>
#include <QDebug>

#include "kohdennus.h"



Kohdennus::Kohdennus(int tyyppi)
    : KantaVariantti(), tyyppi_(tyyppi)
{

}

Kohdennus::Kohdennus(QVariantMap &data) :
    KantaVariantti(data)
{
    qDebug() << "Kohdennus " << data;

    id_ = data_.take("id").toInt();

    tyyppi_ = data_.take("tyyppi").toInt();
    nimi_.aseta( data_.take("nimi"));
    kuuluu_ = data.take("kuuluu").toInt();
    vienteja_ = data.take("lkm").toInt();
    vienteja_ += data.take("mlkm").toInt();
}

QIcon Kohdennus::tyyppiKuvake() const
{
    if( tyyppi() == KUSTANNUSPAIKKA)
        return QIcon(":/pic/kohdennus.png");
    else if( tyyppi() == PROJEKTI )
        return QIcon(":/pic/projekti.png");
    else if( tyyppi() == MERKKAUS )
        return QIcon(":/pic/tag.png");
    else
        return QIcon();
}


void Kohdennus::asetaId(int id)
{
    id_ = id;
}

void Kohdennus::asetaNimi(const QString &nimi, const QString &kieli)
{
    nimi_.aseta(nimi, kieli);
}

void Kohdennus::asetaTyyppi(Kohdennus::KohdennusTyyppi tyyppi)
{
    tyyppi_ = tyyppi;
    if( tyyppi != PROJEKTI)
        kuuluu_ = 0;
}

void Kohdennus::asetaKuuluu(int kohdennusid)
{
    kuuluu_ = kohdennusid;
}

QVariantMap Kohdennus::data() const
{
    QVariantMap map(KantaVariantti::data());
    map.insert("id", id());
    map.insert("nimi", nimi_.map());
    map.insert("tyyppi", tyyppi());
    if( tyyppi() == PROJEKTI)
        map.insert("kuuluu", kuuluu());
    return map;
}

