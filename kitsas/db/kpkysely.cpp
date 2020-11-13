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
#include "db/kirjanpito.h"
#include "yhteysmodel.h"
#include "tuonti/csvtuonti.h"

#include <QDate>

KpKysely::KpKysely(YhteysModel *parent, KpKysely::Metodi metodi, QString polku) :
    QObject (parent), metodi_(metodi), polku_(polku)
{

}

void KpKysely::asetaUrl(const QString &kysely)
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

void KpKysely::lisaaAttribuutti(const QString &avain, int arvo)
{
    lisaaAttribuutti(avain, QString::number(arvo));
}

QString KpKysely::attribuutti(const QString &avain) const
{
    return kysely_.queryItemValue(avain);
}

QString KpKysely::tiedostotyyppi(const QByteArray &ba)
{
    QByteArray png ;
    png.resize(4);
    png[0] = static_cast<char>( 0x89 );
    png[1] = static_cast<char>(0x50 );
    png[2] = static_cast<char>(0x4e );
    png[3] = static_cast<char>(0x47);

    QByteArray jpg;
    jpg.resize(3);
    jpg[0] =  static_cast<char>( 0xff );
    jpg[1] = static_cast<char>( 0xd8 );
    jpg[2] = static_cast<char>( 0xff );

    if( ba.startsWith("%PDF"))
        return ("application/pdf");
    else if(  ba.startsWith(png) )
        return("image/png");
    else if( ba.startsWith(jpg))
        return("image/jpeg");
    else if( ba.startsWith(QByteArray("<?xml")))
        return("application/xml");    
    else if( Tuonti::CsvTuonti::onkoCsv(ba))
        return("text/csv");

    return "application/octet-stream";
}

