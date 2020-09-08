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
#include "kantavariantti.h"
#include <QDate>

KantaVariantti::KantaVariantti(const QVariantMap &data) :
    data_(data)
{

}

QVariant KantaVariantti::arvo(const QString &avain) const
{
    return data_.value(avain);
}

QString KantaVariantti::str(const QString &avain) const
{
    return arvo(avain).toString();
}

int KantaVariantti::luku(const QString &avain) const
{
    return arvo(avain).toInt();
}

double KantaVariantti::dbl(const QString &avain) const
{
    return arvo(avain).toDouble();
}

QDate KantaVariantti::pvm(const QString &avain) const
{
    return arvo(avain).toDate();
}

void KantaVariantti::clear()
{
    data_.clear();
}

void KantaVariantti::set(const QString &avain, const QVariant &arvo)
{
    data_.insert(avain, arvo);
}

void KantaVariantti::setInt(const QString &avain, int arvo)
{
    if( !arvo)
        data_.remove(avain);
    else
        data_.insert(avain, arvo);
}

void KantaVariantti::setStr(const QString &avain, const QString& arvo)
{
    if( avain.isEmpty() )
        data_.remove(avain);
    else
        data_.insert(avain, arvo);
}

void KantaVariantti::unset(const QString &avain)
{
    data_.remove(avain);
}

