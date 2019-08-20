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
#include "tasetulosraportti.h"
#include "db/kirjanpito.h"

#include <QJsonDocument>
#include <QStringListModel>
#include <QDebug>

TaseTulosRaportti::TaseTulosRaportti(Raportoija::RaportinTyyppi raportinTyyppi, QWidget *parent) :
    Raportti (parent),
    ui( new Ui::MuokattavaRaportti ),
    tyyppi_(raportinTyyppi)

{
    ui->setupUi( raporttiWidget );

    QStringList muodot;
    if(tyyppi() == Raportoija::TASE )
        muodot = kp()->asetukset()->avaimet("tase/");
    else if( tyyppi() == Raportoija::TULOSLASKELMA)
             muodot = kp()->asetukset()->avaimet("tulos/");

    connect( ui->muotoCombo, &QComboBox::currentTextChanged, this, &TaseTulosRaportti::paivitaKielet);

    for( auto muoto : muodot ) {
        QString kaava = kp()->asetukset()->asetus(muoto);
        QJsonDocument doc = QJsonDocument::fromJson( kaava.toUtf8() );
        QVariantMap map = doc.toVariant().toMap().value("muoto").toMap();
        QString muotonimi = map.value("fi").toString();
        ui->muotoCombo->addItem( muotonimi, muoto );
    }

    if( tyyppi()==Raportoija::TULOSLASKELMA && kp()->kohdennukset()->kohdennuksia())
    {
        ui->kohdennusCombo->setModel( kp()->kohdennukset());
        ui->kohdennusCombo->setModelColumn( KohdennusModel::NIMI);
    }
    else
    {
        ui->kohdennusCheck->setVisible(false);
        ui->kohdennusCombo->setVisible(false);
    }

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

void TaseTulosRaportti::esikatsele()
{
    Raportoija *raportoija = new Raportoija( ui->muotoCombo->currentData().toString(),
                                             ui->kieliCombo->currentData().toString(),
                                             this);

    if( ui->kohdennusCheck->isChecked())
        raportoija->lisaaKohdennus( ui->kohdennusCombo->currentData(KohdennusModel::IdRooli).toInt() );

    if( raportoija->onkoKausiraportti())
    {
        raportoija->lisaaKausi( ui->alkaa1Date->date(), ui->loppuu1Date->date(), ui->tyyppi1->currentIndex());
        if( ui->sarake2Box->isChecked())
            raportoija->lisaaKausi( ui->alkaa2Date->date(), ui->loppuu2Date->date(), ui->tyyppi2->currentIndex());
        if( ui->sarake3Box->isChecked())
            raportoija->lisaaKausi( ui->alkaa3Date->date(), ui->loppuu3Date->date(), ui->tyyppi3->currentIndex());
        if( ui->sarake4Box->isChecked())
            raportoija->lisaaKausi( ui->alkaa4Date->date(), ui->loppuu4Date->date(), ui->tyyppi4->currentIndex());
    }
    else
    {
        raportoija->lisaaTasepaiva( ui->loppuu1Date->date());
        if( ui->sarake2Box->isChecked())
            raportoija->lisaaTasepaiva( ui->loppuu2Date->date());
        if( ui->sarake3Box->isChecked())
            raportoija->lisaaTasepaiva( ui->loppuu3Date->date());
        if( ui->sarake4Box->isChecked())
            raportoija->lisaaTasepaiva( ui->loppuu4Date->date());
    }

//    if( raportoija->tyyppi() == Raportoija::KOHDENNUSLASKELMA && !ui->kohdennusCheck->isChecked())
//        raportoija.etsiKohdennukset();

    connect( raportoija, &Raportoija::valmis, this, &Raportti::nayta);
    raportoija->kirjoita( ui->erittelyCheck->isChecked() );

}

void TaseTulosRaportti::paivitaKielet()
{
    QString raportti = ui->muotoCombo->currentData().toString();
    QString kaava = kp()->asetukset()->asetus(raportti);
    QJsonDocument doc = QJsonDocument::fromJson( kaava.toUtf8() );
    QVariantMap nimet = doc.toVariant().toMap().value("nimi").toMap();

    ui->kieliCombo->clear();

    for(auto kieli : nimet.keys()) {
        if( kieli == "fi")
            ui->kieliCombo->addItem( QIcon(":/liput/fi.png"), tr("Suomi"), "fi");
        else if( kieli == "sv")
            ui->kieliCombo->addItem( QIcon(":/liput/se.png"), tr("Ruotsi"), "sv");
    }

}

void TaseTulosRaportti::paivitaUi()
{
    // Jos tehdään Raportoija::TASElaskelmaa, piilotetaan turhat tiedot!
    ui->alkaa1Date->setVisible( tyyppi() != Raportoija::TASE );
    ui->alkaa2Date->setVisible( tyyppi() != Raportoija::TASE );
    ui->alkaa3Date->setVisible( tyyppi() != Raportoija::TASE );
    ui->alkaa4Date->setVisible( tyyppi() != Raportoija::TASE );
    ui->alkaaLabel->setVisible( tyyppi() != Raportoija::TASE );
    ui->paattyyLabel->setVisible( tyyppi() != Raportoija::TASE );

    ui->tyyppi1->setVisible( tyyppi() != Raportoija::TASE);
    ui->tyyppi2->setVisible( tyyppi() != Raportoija::TASE);
    ui->tyyppi3->setVisible( tyyppi() != Raportoija::TASE);
    ui->tyyppi4->setVisible( tyyppi() != Raportoija::TASE);


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

    // Neljäs sarake oletuksena poissa käytöstä
    // Sitä tarvitaan lähinnä budjettivertailuihin
    if( tilikausiIndeksi > -1)
    {
        ui->alkaa4Date->setDate( kp()->tilikaudet()->tilikausiIndeksilla(tilikausiIndeksi).alkaa() );
        ui->loppuu4Date->setDate( kp()->tilikaudet()->tilikausiIndeksilla(tilikausiIndeksi).paattyy() );
    }
    ui->sarake4Box->setChecked(false);
}
