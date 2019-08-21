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
#include "budjettivertailu.h"
#include "db/kirjanpito.h"
#include "db/tilikausimodel.h"
#include <QComboBox>

#include "raportoija.h"

Budjettivertailu::Budjettivertailu() :
    Raportti(nullptr)
{
    ui_ = new Ui::Budjettivertailu;
    ui_->setupUi(raporttiWidget);

    ui_->kausiCombo->setModel(kp()->tilikaudet());
    ui_->kausiCombo->setModelColumn(TilikausiModel::KAUSI);
    ui_->kausiCombo->setCurrentIndex( kp()->tilikaudet()->indeksiPaivalle( kp()->paivamaara() ) );


    // Lisätään muokattavat raportit
    QStringList raporttilista;

    for (QString rnimi : kp()->asetukset()->avaimet("Raportti/") )
    {
        // Kohdennusraportit kuitenkin vain, jos kohdennuksia käytettävissä
        if( kp()->asetukset()->asetus(rnimi).startsWith(":kohdennus") && !kp()->kohdennukset()->kohdennuksia() )
            continue;
        // Ei tasetita
        if( kp()->asetukset()->asetus(rnimi).startsWith(":tase") )
            continue;

        // Raporttilajit: Jos lajillinen raportti (esim. Tase/PMA, tulee listalle kuitenkin vain Tase yhteen kertaan
        if( rnimi.count(QChar('/')) > 1)
            rnimi = rnimi.left( rnimi.lastIndexOf(QChar('/')) );

        if( !raporttilista.contains(rnimi))
            raporttilista.append(rnimi);
    }
    raporttilista.sort(Qt::CaseInsensitive);
    for( const QString& nimi : raporttilista)
    {
        ui_->raporttiCombo->addItem( QIcon(":/pic/tekstisivu.png"),nimi.mid(9), nimi );
    }

    if( ui_->raporttiCombo->findText("Tuloslaskelma") > -1)
        ui_->raporttiCombo->setCurrentIndex( ui_->raporttiCombo->findText("Tuloslaskelma") );


    if( kp()->kohdennukset()->kohdennuksia())
    {
        ui_->kohdennusCombo->setModel( kp()->kohdennukset());
        ui_->kohdennusCombo->setModelColumn( KohdennusModel::NIMI);
    }
    else
    {
        ui_->kohdennusCheck->setVisible(false);
        ui_->kohdennusCombo->setVisible(false);
    }

    paivitaMuodot();
    connect( ui_->raporttiCombo, SIGNAL( currentIndexChanged(int) ), this, SLOT(paivitaMuodot()));

}

Budjettivertailu::~Budjettivertailu()
{
    delete ui_;
}

RaportinKirjoittaja Budjettivertailu::raportti()
{
    QString raporttiTyyppi = ui_->raporttiCombo->currentData().toString().mid(9);
    if( ui_->muotoCombo->isVisible())
        raporttiTyyppi = ui_->muotoCombo->currentData().toString();

    Raportoija raportoija(raporttiTyyppi);

    Tilikausi kausi = kp()->tilikaudet()->tilikausiIndeksilla( ui_->kausiCombo->currentIndex() );
    raportoija.lisaaKausi( kausi.alkaa(), kausi.paattyy(),  Raportoija::BUDJETTI );
    raportoija.lisaaKausi( kausi.alkaa(), kausi.paattyy(),  Raportoija::TOTEUTUNUT );
    raportoija.lisaaKausi( kausi.alkaa(), kausi.paattyy(),  Raportoija::BUDJETTIERO );
    raportoija.lisaaKausi( kausi.alkaa(), kausi.paattyy(),  Raportoija::TOTEUMAPROSENTTI );

    return RaportinKirjoittaja(); // raportoija.raportti( ui_->erittelyCheck->isChecked());

}

void Budjettivertailu::paivitaMuodot()
{
    QString raporttiTyyppi = ui_->raporttiCombo->currentData().toString();

    QStringList muodot = kp()->asetukset()->avaimet( raporttiTyyppi + '/');

    bool monimuoto = muodot.count();

    ui_->muotoCombo->setVisible( monimuoto);
    ui_->muotoLabel->setVisible( monimuoto);

    ui_->muotoCombo->clear();

    if( monimuoto )
    {
        for( const QString& muoto : muodot)
        {
            ui_->muotoCombo->addItem( muoto.mid(muoto.lastIndexOf(QChar('/'))+1) , muoto.mid(9) );
        }

        // Oletuksena monesta muodosta valittuna Yleinen
        if( ui_->muotoCombo->findText("Yleinen") > -1)
            ui_->muotoCombo->setCurrentIndex( ui_->muotoCombo->findText("Yleinen") );

    }


}
