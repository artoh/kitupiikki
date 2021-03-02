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
#include "kielikentta.h"
#include "db/kirjanpito.h"
#include <QDebug>
#include <QJsonDocument>

#include <QListWidget>

KieliKentta::KieliKentta()
{

}

KieliKentta::KieliKentta(const QVariant &var) :
    moni_(var)
{
}

KieliKentta::KieliKentta(const QString &var) :
    moni_(var)
{
}

void KieliKentta::aseta(const QVariant &var)
{
    moni_.aseta(var);
}

void KieliKentta::aseta(const QString &nimi, const QString &kieli)
{
    moni_.aseta(nimi, kieli);
}

QString KieliKentta::teksti( QString kieli) const
{
    return moni_.teksti(kieli);
}

QString KieliKentta::kaannos(const QString &kieli) const
{
    return moni_.kaannos(kieli);
}

void KieliKentta::tyhjenna()
{
    moni_ = Monikielinen();
}

QVariantMap KieliKentta::map() const
{
    return moni_.map();
}
