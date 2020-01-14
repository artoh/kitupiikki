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

#include "tili.h"

#include <QDebug>
#include <QSqlQuery>

#include "kirjanpito.h"

Tili::Tili() : numero_(0), tila_(1),ylaotsikkoId_(0)
{

}


Tili::Tili(const QVariantMap &data) :
    KantaVariantti (data), tila_(1), ylaotsikkoId_(0)
{

    nimi_.aseta( data_.take("nimi") );
    ohje_.aseta( data_.take("ohje") );

    numero_ = data_.value("numero").toInt();
    tyyppi_ = kp()->tiliTyypit()->tyyppiKoodilla( data_.take("tyyppi").toString() );

    laajuus_ = data_.take("laajuus").toInt();

}

int Tili::id() const
{
    throw QString("Yritetään saada tilin id %1").arg( nimiNumero() );
}

QString Tili::nimi(const QString &kieli) const
{
    return nimi_.teksti(kieli);
}

QString Tili::nimiNumero(const QString &kieli) const
{
    if( onkoValidi())
        return QString("%1 %2").arg(numero()).arg(nimi(kieli));
    else
        return QString();
}

QString Tili::ohje(const QString &kieli) const
{
    return ohje_.teksti(kieli);
}

void Tili::asetaNumero(int numero)
{
    numero_ = numero;
}

void Tili::asetaTyyppi(const QString &tyyppikoodi)
{
    tyyppi_ = kp()->tiliTyypit()->tyyppiKoodilla(tyyppikoodi);
}

void Tili::asetaOtsikko(Tili *otsikko)
{
    tamanOtsikko_ = otsikko;
}

bool Tili::onkoValidi() const
{
    return numero() > 0 ;
}


bool Tili::onko(TiliLaji::TiliLuonne luonne) const
{
    return tyyppi().onko(luonne);
}

QVariantMap Tili::data() const
{
    QVariantMap map( KantaVariantti::data() );
    map.insert("nimi", nimi_.map());
    map.insert("numero", numero());
    map.insert("tyyppi", tyyppiKoodi());
    map.insert("laajuus", laajuus());
    if( !ohje_.map().isEmpty())
        map.insert("ohje", ohje_.map());
    return map;
}


