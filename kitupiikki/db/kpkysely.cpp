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
#include "kpkysely.h"
#include "kpyhteys.h"

#include <QDate>

KpKysely::KpKysely(KpYhteys *parent)
 : QObject (parent), metodi_(GET), tila_(ALUSTUS)
{

}

void KpKysely::lisaaAttribuutti(const QString &avain, const QString &arvo)
{
    kysely_.addQueryItem(avain, arvo);
}

void KpKysely::lisaaAttribuutti(const QString &avain, const QDate &paiva)
{
    lisaaAttribuutti(avain, paiva.toString(Qt::ISODate));
}

QString KpKysely::attribuutti(const QString &avain) const
{
    return kysely_.queryItemValue(avain);
}

void KpKysely::kysy()
{
    yhteys()->kasitteleKysely(this);
}

void KpKysely::vastaa(QVariant arvo, KpKysely::Tila tila)
{
    vastaus_.setValue(arvo);
    emit vastaus(&vastaus_, tila);
}

KpYhteys *KpKysely::yhteys() const
{
    return static_cast<KpYhteys*>( parent() );
}
