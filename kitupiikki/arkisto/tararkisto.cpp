/*
   Copyright (C) 2018 Arto Hyvättinen

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

#include <QFileInfo>
#include <QDebug>
#include <QByteArray>

#include <QDateTime>

#include "tararkisto.h"

TarArkisto::TarArkisto(const QString &polku) :
    QFile( polku )
{


}

TarArkisto::~TarArkisto()
{
    if( isOpen())
        lopeta();
}

bool TarArkisto::aloita()
{
    return open( QIODevice::WriteOnly);
}

bool TarArkisto::lisaaTiedosto(const QString &polku)
{
    QFileInfo info(polku);

    if( !info.exists() || !info.size() )
    {
        qDebug() << "Tiedostoa " << polku << " ei löydy.";
        return false;
    }

    QFile in(polku);
    if( !in.open(QIODevice::ReadOnly))
    {
        qDebug() << "Tiedostoa " << polku << " ei voi avata.";
        return false;
    }

    QByteArray otsake(512, '\0');
    QByteArray tiedostonnimi = info.fileName().toLocal8Bit();

    otsake.replace(0, tiedostonnimi.length(), tiedostonnimi);
    otsake.replace(100, QByteArray("000644 ").size() , QByteArray("000644 "));

    QByteArray koko = QString("%1 ").arg( info.size(), 11, 8, QChar('0') ).toLocal8Bit();
    otsake.replace( 124, koko.size(), koko );

    QByteArray mtime = QString("%1 ").arg( info.fileTime(QFileDevice::FileModificationTime).toSecsSinceEpoch() , 11, 8, QChar('0') ).toLocal8Bit();

    otsake.replace( 136, mtime.size(), mtime);
    otsake.replace( 156, QByteArray("0").size(), QByteArray("0"));

    otsake.replace( 257, QByteArray("ustar").size(), QByteArray("ustar"));
    otsake.replace( 263, QByteArray("00").size(), QByteArray("00"));

    otsake.replace( 297, QByteArray("users").size(), QByteArray("users"));

    otsake.replace( 0x149, QByteArray("000000 ").size(), QByteArray("000000 "));
    otsake.replace( 0x151, QByteArray("000000 ").size(), QByteArray("000000 "));

    quint32 tarkastussumma = 32 * 8;
    // Tarkastussumman laskenta
    for( int i=0; i < otsake.size(); i++)
        tarkastussumma += otsake.at(i);

    QByteArray tarkaste = QString("%1 ").arg( tarkastussumma, 6, 8, QChar('0') ).toLocal8Bit();
    otsake.replace(148, tarkaste.length(), tarkaste);

    // Nyt otsake on valmis kirjoitettavaksi
    write( otsake );

    // Sitten vielä kirjoitetaan tiedosto
    write( in.readAll() );

    // Ja lopuksi täytetään viimeinen tietue
    int jaannos = 512 - info.size() % 512;
    QByteArray tyhja( jaannos, '\0');
    write( tyhja );

    qDebug() << info.fileName() << " " << info.size();

    return true;

}

void TarArkisto::lopeta()
{
    // Kirjoitetaan kaksi tyhjää blokkia
    QByteArray blokki(512, '\0');
    write( blokki );
    write( blokki );

    close();
}
