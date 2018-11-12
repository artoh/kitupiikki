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


#include "palkkafituonti.h"
#include "db/vientimodel.h"
#include "db/kirjanpito.h"
#include "kirjaus/kirjauswg.h"

#include <QString>


PalkkaFiTuonti::PalkkaFiTuonti(KirjausWg *wg)
    : Tuonti(wg)
{
    // Haetaan muuntotaulukko
    QStringList muunnot = kp()->asetukset()->lista("PalkkaFiTuonti");
    for( const QString& muunto : muunnot)
    {
        int vali = muunto.indexOf(' ');
        muunto_.insert( muunto.leftRef(vali).toInt(), muunto.midRef(vali+1).toInt()  );
    }
}

bool PalkkaFiTuonti::tuo(const QByteArray &data)
{
    QString utf8 = QString::fromUtf8(data);
    QStringList rivit = utf8.split('\n');

    if( rivit.length() < 2)
        return false;

    // Haetaan ensimmäinen rivi
    QStringList ekarivi = rivit.takeFirst().split(';');

    if( ekarivi.length() < 9 )
        return false;

    // Tältä riviltä haetaan päättymispäivä, josta tulee tositepäivä
    pvm_ = QDate::fromString( ekarivi.at(6), "dd.MM.yyyy" );

    otsikko_ = kp()->tr("Palkat %1 - %2").arg(ekarivi.at(5))
                                    .arg(ekarivi.at(6));

    kirjausWg()->model()->asetaOtsikko( otsikko_ );
    kirjausWg()->model()->asetaTositelaji(1);   // Muu tosite
    kirjausWg()->model()->asetaKommentti( kp()->tr("Palkka.fi kirjanpitotosite %1")
                                          .arg( ekarivi.at(3) ));

    while( rivit.length())
        tuoRivi( rivit.takeFirst() );

    kirjausWg()->tiedotModelista();

    return true;
}

void PalkkaFiTuonti::tuoRivi(const QString &rivi)
{
    QStringList kentat = rivi.split(';');
    if( kentat.length() < 5 || kentat.first() != "V")
        return;

    int tilinumero = kentat.at(1).toInt();

    QString euro = kentat.at(3);
    int pilkunpaikka = euro.indexOf(',');

    // Koska vain euro-osa on negatiivinen, käsitellään eurojen ja
    // senttien osuus erikseen
    qlonglong eurot = euro.left( pilkunpaikka ).toLongLong();
    int sentit = euro.mid( pilkunpaikka + 1).toInt();

    VientiRivi vienti;
    vienti.pvm = pvm_;

    if( tilinumero == 1910)
        vienti.selite = otsikko_;
    else
        vienti.selite = otsikko_ + " " + kentat.value(2);

    vienti.tili = kp()->tilit()->tiliNumerolla( muunto_.value( tilinumero, tilinumero ) );

    if( eurot > 0 )
        vienti.debetSnt = eurot * 100 + sentit;
    else
        vienti.kreditSnt = 0 - eurot * 100 + sentit;


    kirjausWg()->model()->vientiModel()->lisaaVienti(vienti);

}
