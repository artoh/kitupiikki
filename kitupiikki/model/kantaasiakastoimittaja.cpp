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
#include "kantaasiakastoimittaja.h"

KantaAsiakasToimittaja::KantaAsiakasToimittaja(QObject *parent) : QObject(parent)
{

}


QString KantaAsiakasToimittaja::maa() const
{
    QString maa = data_.value("maa").toString();
    if( maa.isEmpty())
        return "fi";
    return maa;
}

QString KantaAsiakasToimittaja::ytunnus() const
{
    QString tunnus = data_.value("alvtunnus").toString();
    if( !tunnus.startsWith("FI"))
        return QString();

    tunnus.remove(0,2);
    tunnus.insert(7,'-');
    return tunnus;
}

void KantaAsiakasToimittaja::set(const QString &avain, const QString &arvo)
{
    if( arvo.isEmpty())
        data_.remove(avain);
    else
        data_.insert(avain, arvo);

}

void KantaAsiakasToimittaja::setYTunnus(const QString &tunnus)
{
    QString tunnari = tunnus;
    if( !tunnari.isEmpty())
        data_.insert( "alvtunnus", tunnari.remove('-').insert(0,"FI") );
}

void KantaAsiakasToimittaja::tallennusvalmis(QVariant *var)
{    
    QVariantMap map = var->toMap();
    int id = map.value("id").toInt();
    data_.insert("id", id);
    emit tallennettu( id );
}
