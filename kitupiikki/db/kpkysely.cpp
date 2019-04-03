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
#include "db/kirjanpito.h"

#include <QDate>

KpKysely::KpKysely(KpYhteys *parent, KpKysely::Metodi metodi, QString polku) :
    QObject (parent), metodi_(metodi), polku_(polku)
{

}

void KpKysely::asetaKysely(const QString &kysely)
{
    QUrl url(kysely);
    polku_ = url.path();
    kysely_ = QUrlQuery( url );
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


void KpKysely::vastaa(KpKysely::Tila tila)
{
    QMapIterator<QString, QVariant> iter( vastaus_ );
    while (iter.hasNext()) {
        iter.next();
        QString avain = iter.key();

        if( avain == "asetukset")
            kp()->asetukset()->lataa( iter.value().toList() );
        else if( avain == "tilit")
            kp()->tilit()->lataa( iter.value().toList() );
        else if( avain == "kohdennukset")
            kp()->kohdennukset()->lataa( iter.value().toList() );
        else if( avain == "tositelajit")
            kp()->tositelajit()->lataa( iter.value().toList() );
        else if( avain == "tilikaudet")
            kp()->tilikaudet()->lataa( iter.value().toList() );
    }

    emit vastaus( &vastaus_, tila );

}
