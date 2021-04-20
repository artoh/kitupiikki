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
#include "tuote.h"

Tuote::Tuote()
{

}

Tuote::Tuote(const QVariantMap &map)
{
    id_ = map.value("id").toInt();
    nimike_ = map.value("nimike").toString();
    yksikko_ = map.value("yksikko").toString();
    unKoodi_ = map.value("UNkoodi").toString();
    ahinta_ = map.value("ahinta").toDouble();
    kohdennus_ = map.value("kohdennus").toInt();
    tili_ = map.value("tili").toInt();
    alvkoodi_ = map.value("alvkoodi").toInt();
    alvprosentti_ = map.value("alvprosentti").toDouble();
    if( map.value("nimi").toMap().isEmpty() && !nimike_.isEmpty())
        nimikielinen_.aseta("fi", nimike_);
    else
        nimikielinen_.aseta(map.value("nimi"));

}

QVariantMap Tuote::toMap() const
{
    QVariantMap map;
    map.insert("id", id());
    map.insert("nimike", nimike());
    if(!yksikko().isEmpty())
        map.insert("yksikko", yksikko());
    if(!unKoodi().isEmpty())
        map.insert("UNkoodi", unKoodi());
    map.insert("ahinta", ahinta());
    if( kohdennus())
        map.insert("kohdennus", kohdennus());
    map.insert("tili", tili());
    map.insert("alvkoodi", alvkoodi());
    if( qAbs(alvprosentti()) > 1e-5)
        map.insert("alvprosentti", alvprosentti());
    map.insert("nimi", nimikielinen_.map());
    return map;
}

double Tuote::alvprosentti() const
{
    return alvprosentti_;
}

void Tuote::setAlvprosentti(double alvprosentti)
{
    alvprosentti_ = alvprosentti;
}

int Tuote::alvkoodi() const
{
    return alvkoodi_;
}

void Tuote::setAlvkoodi(int alvkoodi)
{
    alvkoodi_ = alvkoodi;
}

int Tuote::tili() const
{
    return tili_;
}

void Tuote::setTili(int tili)
{
    tili_ = tili;
}

int Tuote::kohdennus() const
{
    return kohdennus_;
}

void Tuote::setKohdennus(int kohdennus)
{
    kohdennus_ = kohdennus;
}

double Tuote::ahinta() const
{
    return ahinta_;
}

void Tuote::setAhinta(double ahinta)
{
    ahinta_ = ahinta;
}

QString Tuote::unKoodi() const
{
    return unKoodi_;
}

void Tuote::setUnKoodi(const QString &unKoodi)
{
    unKoodi_ = unKoodi;
    if( !unKoodi.isEmpty())
        yksikko_.clear();
}

QString Tuote::yksikko() const
{
    return yksikko_;
}

void Tuote::setYksikko(const QString &yksikko)
{
    yksikko_ = yksikko;
    if( !yksikko.isEmpty())
        unKoodi_.clear();
}

QString Tuote::nimike(const QString& kieli) const
{
    QString nimi = nimikielinen_.teksti(kieli);
    if( nimi.isEmpty())
        return nimike_;
    else
        return nimi;
}

void Tuote::setNimike(const QString &nimike)
{
    nimike_ = nimike;
}

Monikielinen &Tuote::nimiKielinen()
{
    return nimikielinen_;
}

Monikielinen Tuote::constKielinen() const
{
    return nimikielinen_;
}

int Tuote::id() const
{
    return id_;
}

void Tuote::setId(int id)
{
    id_ = id;
}
