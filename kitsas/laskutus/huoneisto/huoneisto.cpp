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
#include "huoneisto.h"
#include "db/kirjanpito.h"
#include "huoneistomodel.h"

Huoneisto::Huoneisto(QObject *parent) : QObject(parent), laskutus_( kp(), this )
{

}

void Huoneisto::lataa(int id)
{
    KpKysely *kysely = kpk(QString("/huoneistot/%1").arg(id));
    connect(kysely, &KpKysely::vastaus, this, &Huoneisto::lataaData);
    kysely->kysy();
}

void Huoneisto::tallenna()
{
    KpKysely *kysely;
    if( id())
        kysely = kpk(QString("/huoneistot/%1").arg(id()), KpKysely::PUT);
    else
        kysely = kpk("/huoneistot", KpKysely::POST);
    connect( kysely, &KpKysely::vastaus, this, &Huoneisto::tallennusValmis);
    kysely->kysy(toMap());
}

HuoneistoLaskutusModel *Huoneisto::laskutus()
{
    return &laskutus_;
}

void Huoneisto::lataaData(QVariant *data)
{
    QVariantMap map = data->toMap();
    id_ = map.value("id").toInt();
    nimi_ = map.value("nimi").toString();
    asiakas_ = map.value("asiakas").toInt();
    muistiinpanot_ = map.value("muistiinpanot").toString();
    laskutus_.lataa(map.value("laskutus").toList());
    emit ladattu();
}

QVariantMap Huoneisto::toMap() const
{
    QVariantMap map;
    map.insert("id", id());
    map.insert("nimi", nimi());
    if(asiakas())
        map.insert("asiakas", asiakas());
    if(!muistiinpanot().isEmpty())
        map.insert("muistiinpanot", muistiinpanot());
    map.insert("laskutus", laskutus_.list());
    return map;
}

void Huoneisto::tallennusValmis()
{
    kp()->huoneistot()->paivita();
    emit tallennettu();
}

QString Huoneisto::muistiinpanot() const
{
    return muistiinpanot_;
}

void Huoneisto::setMuistiinpanot(const QString &muistiinpanot)
{
    muistiinpanot_ = muistiinpanot;
}

QString Huoneisto::nimi() const
{
    return nimi_;
}

void Huoneisto::setNimi(const QString &nimi)
{
    nimi_ = nimi;
}

int Huoneisto::asiakas() const
{
    return asiakas_;
}

void Huoneisto::setAsiakas(int asiakas)
{
    asiakas_ = asiakas;
}

int Huoneisto::id() const
{
    return id_;
}

