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

#include "muokattavaraportti.h"
#include "db/kirjanpito.h"
#include "db/tilikausi.h"

#include <QSqlQuery>

#include <QDebug>


MuokattavaRaportti::MuokattavaRaportti(const QString &raporttinimi)
    : Raportti(), raporttiNimi(raporttinimi)
{
    ui = new Ui::MuokattavaRaportti;
    ui->setupUi( raporttiWidget );

    Raportoija raportoija(raporttinimi);

    // Jos tehdään taselaskelmaa, piilotetaan turhat tiedot!
    ui->alkaa1Date->setVisible( raportoija.onkoKausiraportti() );
    ui->alkaa2Date->setVisible( raportoija.onkoKausiraportti() );
    ui->alkaa3Date->setVisible( raportoija.onkoKausiraportti() );
    ui->alkaaLabel->setVisible( raportoija.onkoKausiraportti() );
    ui->paattyyLabel->setVisible( raportoija.onkoKausiraportti() );

    // Sitten laitetaan valmiiksi tilikausia nykyisestä taaksepäin
    int tilikausiIndeksi = kp()->tilikaudet()->indeksiPaivalle( kp()->paivamaara() );
    if( tilikausiIndeksi > -1 )
    {
        ui->alkaa1Date->setDate( kp()->tilikaudet()->tilikausiIndeksilla(tilikausiIndeksi).alkaa() );
        ui->loppuu1Date->setDate( kp()->tilikaudet()->tilikausiIndeksilla(tilikausiIndeksi).paattyy() );
    }
    if( tilikausiIndeksi > 0)
    {
        ui->alkaa2Date->setDate( kp()->tilikaudet()->tilikausiIndeksilla(tilikausiIndeksi-1).alkaa() );
        ui->loppuu2Date->setDate( kp()->tilikaudet()->tilikausiIndeksilla(tilikausiIndeksi-1).paattyy() );
    }
    ui->sarake2Box->setChecked(tilikausiIndeksi > 0);

    if( tilikausiIndeksi > 1)
    {
        ui->alkaa3Date->setDate( kp()->tilikaudet()->tilikausiIndeksilla(tilikausiIndeksi-2).alkaa() );
        ui->loppuu3Date->setDate( kp()->tilikaudet()->tilikausiIndeksilla(tilikausiIndeksi-2).paattyy() );
    }
    ui->sarake3Box->setChecked(tilikausiIndeksi > 1);

}

MuokattavaRaportti::~MuokattavaRaportti()
{
    delete ui;

}

RaportinKirjoittaja MuokattavaRaportti::raportti()
{    
    Raportoija raportoija( raporttiNimi );

    if( raportoija.onkoKausiraportti())
    {
        raportoija.lisaaKausi( ui->alkaa1Date->date(), ui->loppuu1Date->date());
        if( ui->sarake2Box->isChecked())
            raportoija.lisaaKausi( ui->alkaa2Date->date(), ui->loppuu2Date->date());
        if( ui->sarake3Box->isChecked())
            raportoija.lisaaKausi( ui->alkaa3Date->date(), ui->loppuu3Date->date());
    }
    else
    {
        raportoija.lisaaTasepaiva( ui->loppuu1Date->date());
        if( ui->sarake2Box->isChecked())
            raportoija.lisaaTasepaiva( ui->loppuu2Date->date());
        if( ui->sarake3Box->isChecked())
            raportoija.lisaaTasepaiva( ui->loppuu3Date->date());
    }

    if( raportoija.tyyppi() == Raportoija::KOHDENNUSLASKELMA)
        raportoija.etsiKohdennukset();

    return raportoija.raportti( ui->erittelyCheck->isChecked());
}

