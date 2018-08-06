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
#include <QStringListModel>
#include <QDebug>


MuokattavaRaportti::MuokattavaRaportti(const QString &raporttinimi)
    : Raportti(), raporttiNimi(raporttinimi)
{
    ui = new Ui::MuokattavaRaportti;
    ui->setupUi( raporttiWidget );

    QStringList muodot = kp()->asetukset()->avaimet("Raportti/" + raporttinimi + '/');

    monimuoto = muodot.count();

    ui->muotoCombo->setVisible( monimuoto);
    ui->muotoLabel->setVisible( monimuoto);

    if( monimuoto)
    {
        for( QString muoto : muodot)
        {
            ui->muotoCombo->addItem( muoto.mid(muoto.lastIndexOf(QChar('/'))+1) , muoto.mid(9) );
        }

        // Oletuksena monesta muodosta valittuna Yleinen
        if( ui->muotoCombo->findText("Yleinen") > -1)
            ui->muotoCombo->setCurrentIndex( ui->muotoCombo->findText("Yleinen") );

        connect( ui->muotoCombo, SIGNAL(currentIndexChanged(int)), this, SLOT(paivitaUi()));
    }

    ui->kohdennusCombo->setModel(kp()->kohdennukset());

    QStringList tyyppiLista;
    tyyppiLista << tr("Totetunut") << tr("Budjetti") << tr("Budjettiero €") << tr("Toteutunut %");
    QStringListModel *tyyppiListaModel = new QStringListModel(this);
    tyyppiListaModel->setStringList(tyyppiLista);

    ui->tyyppi1->setModel(tyyppiListaModel);
    ui->tyyppi2->setModel(tyyppiListaModel);
    ui->tyyppi3->setModel(tyyppiListaModel);
    ui->tyyppi4->setModel(tyyppiListaModel);

    // Jos alkupäivämäärä on tilikauden aloittava, päivitetään myös päättymispäivä tilikauden päättäväksi
    connect( ui->alkaa1Date, &QDateEdit::dateChanged, [this](const QDate& date){  if( kp()->tilikaudet()->tilikausiPaivalle(date).alkaa() == date) this->ui->loppuu1Date->setDate( kp()->tilikaudet()->tilikausiPaivalle(date).paattyy() );  });
    connect( ui->alkaa2Date, &QDateEdit::dateChanged, [this](const QDate& date){  if( kp()->tilikaudet()->tilikausiPaivalle(date).alkaa() == date) this->ui->loppuu2Date->setDate( kp()->tilikaudet()->tilikausiPaivalle(date).paattyy() );  });
    connect( ui->alkaa3Date, &QDateEdit::dateChanged, [this](const QDate& date){  if( kp()->tilikaudet()->tilikausiPaivalle(date).alkaa() == date) this->ui->loppuu3Date->setDate( kp()->tilikaudet()->tilikausiPaivalle(date).paattyy() );  });
    connect( ui->alkaa4Date, &QDateEdit::dateChanged, [this](const QDate& date){  if( kp()->tilikaudet()->tilikausiPaivalle(date).alkaa() == date) this->ui->loppuu4Date->setDate( kp()->tilikaudet()->tilikausiPaivalle(date).paattyy() );  });

    paivitaUi();

}

MuokattavaRaportti::~MuokattavaRaportti()
{
    delete ui;

}

RaportinKirjoittaja MuokattavaRaportti::raportti()
{    
    Raportoija raportoija( raporttiNimi );

    if( ui->kohdennusCheck->isChecked())
        raportoija.lisaaKohdennus( ui->kohdennusCombo->currentData(KohdennusModel::IdRooli).toInt() );

    if( raportoija.onkoKausiraportti())
    {
        raportoija.lisaaKausi( ui->alkaa1Date->date(), ui->loppuu1Date->date(), ui->tyyppi1->currentIndex());
        if( ui->sarake2Box->isChecked())
            raportoija.lisaaKausi( ui->alkaa2Date->date(), ui->loppuu2Date->date(), ui->tyyppi2->currentIndex());
        if( ui->sarake3Box->isChecked())
            raportoija.lisaaKausi( ui->alkaa3Date->date(), ui->loppuu3Date->date(), ui->tyyppi3->currentIndex());
        if( ui->sarake4Box->isChecked())
            raportoija.lisaaKausi( ui->alkaa4Date->date(), ui->loppuu4Date->date(), ui->tyyppi4->currentIndex());
    }
    else
    {
        raportoija.lisaaTasepaiva( ui->loppuu1Date->date());
        if( ui->sarake2Box->isChecked())
            raportoija.lisaaTasepaiva( ui->loppuu2Date->date());
        if( ui->sarake3Box->isChecked())
            raportoija.lisaaTasepaiva( ui->loppuu3Date->date());
        if( ui->sarake4Box->isChecked())
            raportoija.lisaaTasepaiva( ui->loppuu4Date->date());
    }

    if( raportoija.tyyppi() == Raportoija::KOHDENNUSLASKELMA && !ui->kohdennusCheck->isChecked())
        raportoija.etsiKohdennukset();

    return raportoija.raportti( ui->erittelyCheck->isChecked());
}

void MuokattavaRaportti::paivitaUi()
{
    if( monimuoto)
        raporttiNimi = ui->muotoCombo->currentData(Qt::UserRole).toString();


    Raportoija raportoija(raporttiNimi);

    // Jos tehdään taselaskelmaa, piilotetaan turhat tiedot!
    ui->alkaa1Date->setVisible( raportoija.onkoKausiraportti() );
    ui->alkaa2Date->setVisible( raportoija.onkoKausiraportti() );
    ui->alkaa3Date->setVisible( raportoija.onkoKausiraportti() );
    ui->alkaa4Date->setVisible( raportoija.onkoKausiraportti() );
    ui->alkaaLabel->setVisible( raportoija.onkoKausiraportti() );
    ui->paattyyLabel->setVisible( raportoija.onkoKausiraportti() );

    ui->tyyppi1->setVisible( raportoija.onkoKausiraportti());
    ui->tyyppi2->setVisible( raportoija.onkoKausiraportti());
    ui->tyyppi3->setVisible( raportoija.onkoKausiraportti());
    ui->tyyppi4->setVisible( raportoija.onkoKausiraportti());


    // Sitten laitetaan valmiiksi tilikausia nykyisestä taaksepäin
    int tilikausiIndeksi = kp()->tilikaudet()->indeksiPaivalle( kp()->paivamaara() );
    // #160 Tai sitten viimeinen tilikausi
    if( tilikausiIndeksi < 0)
        tilikausiIndeksi = kp()->tilikaudet()->rowCount(QModelIndex()) - 1;

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
    else if( tilikausiIndeksi > -1)
    {
        ui->alkaa2Date->setDate( kp()->tilikaudet()->tilikausiIndeksilla(tilikausiIndeksi).alkaa() );
        ui->loppuu2Date->setDate( kp()->tilikaudet()->tilikausiIndeksilla(tilikausiIndeksi).paattyy() );
    }


    ui->sarake2Box->setChecked(tilikausiIndeksi > 0);

    if( tilikausiIndeksi > 1)
    {
        ui->alkaa3Date->setDate( kp()->tilikaudet()->tilikausiIndeksilla(tilikausiIndeksi-2).alkaa() );
        ui->loppuu3Date->setDate( kp()->tilikaudet()->tilikausiIndeksilla(tilikausiIndeksi-2).paattyy() );
    }
    else if( tilikausiIndeksi > -1)
    {
        ui->alkaa3Date->setDate( kp()->tilikaudet()->tilikausiIndeksilla(tilikausiIndeksi).alkaa() );
        ui->loppuu3Date->setDate( kp()->tilikaudet()->tilikausiIndeksilla(tilikausiIndeksi).paattyy() );
    }


    ui->sarake3Box->setChecked(tilikausiIndeksi > 1);

    if( tilikausiIndeksi > 2)
    {
        ui->alkaa4Date->setDate( kp()->tilikaudet()->tilikausiIndeksilla(tilikausiIndeksi-3).alkaa() );
        ui->loppuu4Date->setDate( kp()->tilikaudet()->tilikausiIndeksilla(tilikausiIndeksi-3).paattyy() );
    }
    else if( tilikausiIndeksi > -1)
    {
        ui->alkaa4Date->setDate( kp()->tilikaudet()->tilikausiIndeksilla(tilikausiIndeksi).alkaa() );
        ui->loppuu4Date->setDate( kp()->tilikaudet()->tilikausiIndeksilla(tilikausiIndeksi).paattyy() );
    }


    ui->sarake4Box->setChecked(tilikausiIndeksi > 2);
}

