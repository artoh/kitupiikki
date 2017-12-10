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

#include <QDialog>

#include <QDesktopServices>
#include <QUrl>
#include <QProgressDialog>

#include <QMessageBox>

#include "arkistosivu.h"
#include "db/kirjanpito.h"
#include "ui_lisaatilikausidlg.h"
#include "ui_lukitsetilikausi.h"

#include "arkistoija/arkistoija.h"
#include "tilinpaatoseditori/tilinpaatoseditori.h"
#include "tilinpaatoseditori/tpaloitus.h"


ArkistoSivu::ArkistoSivu()
{
    ui = new Ui::TilikausiMaaritykset;
    ui->setupUi(this);

    connect( ui->uusiNappi, SIGNAL(clicked(bool)), this, SLOT(uusiTilikausi()));
    connect( ui->arkistoNappi, SIGNAL(clicked(bool)), this, SLOT(arkisto()));
    connect( ui->tilinpaatosNappi, SIGNAL(clicked(bool)), this, SLOT(tilinpaatos()));

}

ArkistoSivu::~ArkistoSivu()
{
    delete ui;
}

void ArkistoSivu::siirrySivulle()
{
    ui->view->setModel( kp()->tilikaudet() );
    ui->view->resizeColumnsToContents();

    for(int i=0; i < 6; i++)
        ui->view->setColumnWidth( i, (ui->view->width()-10) / 6);
    ui->view->horizontalHeader()->setStretchLastSection(true);

    connect( ui->view->selectionModel() , SIGNAL(currentRowChanged(QModelIndex,QModelIndex)), this, SLOT(nykyinenVaihtuuPaivitaNapit()) );

    ui->view->selectRow( ui->view->model()->rowCount(QModelIndex()) - 1);
}

bool ArkistoSivu::poistuSivulta()
{
    return true;
}

void ArkistoSivu::uusiTilikausi()
{
    Ui::UusiTilikausiDlg dlgUi;
    QDialog dlg;
    dlgUi.setupUi( &dlg );

    QDate edellinen = kp()->tilikaudet()->kirjanpitoLoppuu();
    dlgUi.alkaaEdit->setDate( edellinen.addDays(1) );

    dlgUi.paattyyEdit->setMinimumDate( edellinen.addDays(2) );
    dlgUi.paattyyEdit->setMaximumDate( edellinen.addMonths(18));
    dlgUi.paattyyEdit->setDate( edellinen.addYears(1));

    if( dlg.exec() )
    {
        Tilikausi uusitilikausi( dlgUi.alkaaEdit->date(), dlgUi.paattyyEdit->date() );
        kp()->tilikaudet()->lisaaTilikausi( uusitilikausi );
        kp()->tilikaudet()->tallenna();
    }

}

void ArkistoSivu::arkisto()
{
    if( ui->view->currentIndex().isValid())
    {
        Tilikausi kausi = kp()->tilikaudet()->tilikausiIndeksilla( ui->view->currentIndex().row() );

        // Tehdään arkisto, jos se on päivittämisen tarpeessa
        if( !kausi.arkistoitu().isValid() || kausi.arkistoitu() < kausi.viimeinenPaivitys())
        {
            teeArkisto(kausi);
        }
        // Avataan arkistoi

        QDesktopServices::openUrl( QUrl::fromLocalFile( kp()->hakemisto().absoluteFilePath("arkisto/" + kausi.arkistoHakemistoNimi()) + "/index.html" ));

    }
}

void ArkistoSivu::tilinpaatos()
{
    // Tilinpäätöstoimet
    // - tilinpäätöksen näyttäminen (jos laadittu ja vahvistettu)
    // - tilinpäätöksen muokkaaminen tai vahvistaminen (jos laatiminen aloitettu)
    // - tilinpäätöksen aloittaminen


    if( ui->view->currentIndex().isValid())
    {
        Tilikausi kausi = kp()->tilikaudet()->tilikausiIndeksilla( ui->view->currentIndex().row() );

        if( kausi.tilinpaatoksenTila() == Tilikausi::ALOITTAMATTA)
        {
            if( kp()->paivamaara().daysTo( kausi.paattyy()) >= 0 )
            {
                // Varmistus sille, ettei kirjanpitoa aloiteta jos se on kesken
                if( QMessageBox::question(this, tr("Tilinpäätöksen laatiminen"),
                                          tr("Tilikausi on vielä kesken\n"
                                             "Tilinpäätöksen laatiminen päättää kirjanpidon niin,"
                                             "ettei kirjauksia voi enää lisätä tai muokata.\n"
                                             "Oletko varma, että kaikki tälle tilikaudelle kuuluvat "
                                             "kirjaukset on jo tehty?")) != QMessageBox::Yes)
                    return;
            }

            if( kp()->tilitpaatetty().daysTo( kausi.paattyy() ) > 0)
            {
                // Sitten kirjanpidon lukitseminen ja siihen liittyvä varoitus
                QDialog dlg;
                Ui::LukitseTilikausi ui;
                ui.setupUi( &dlg );
                if( dlg.exec() != QDialog::Accepted)
                    return;

                // Lukitaan tilikausi!
                kp()->asetukset()->aseta("TilitPaatetty", kausi.paattyy());
                // Laaditaan arkisto
                teeArkisto(kausi);
            }

            // Vaihdetaan arkiston tilaa
            kp()->tilikaudet()->vaihdaTilinpaatostila(ui->view->currentIndex().row() ,  Tilikausi::KESKEN);
        }

        kausi = kp()->tilikaudet()->tilikausiIndeksilla( ui->view->currentIndex().row() );
        if( kausi.tilinpaatoksenTila() == Tilikausi::KESKEN)
        {

            // Muokataan tilinpäätöstä
            TilinpaatosEditori *tpEditori = new TilinpaatosEditori( kausi );
            tpEditori->move( window()->x()+50, window()->y()+50);
            tpEditori->show();
            tpEditori->resize( window()->width()-100, window()->height()-100 );

        }
        else if( kausi.tilinpaatoksenTila() == Tilikausi::VAHVISTETTU)
        {
            // Avataan tilinpäätös
            QDesktopServices::openUrl( QUrl::fromLocalFile( kp()->hakemisto().absoluteFilePath("arkisto/" + kausi.arkistoHakemistoNimi()) + "/tilinpaatos.pdf" ));

        }


    }
}

void ArkistoSivu::nykyinenVaihtuuPaivitaNapit()
{
    if( ui->view->currentIndex().isValid())
    {
        Tilikausi kausi = kp()->tilikaudet()->tilikausiIndeksilla( ui->view->currentIndex().row() );
        // Tilikaudelle voi tehdä tilinpäätöksen, jos se ei ole tilinavaus
        ui->tilinpaatosNappi->setEnabled( kausi.tilinpaatoksenTila() != Tilikausi::EILAADITATILINAVAUKSELLE );
        // Tilikauden voi arkistoida, jos tilikautta ei ole lukittu - arkiston voi näyttää aina
        ui->arkistoNappi->setEnabled( kausi.paattyy() > kp()->tilitpaatetty() || kausi.arkistoitu().isValid());
    }
    else
    {
        ui->tilinpaatosNappi->setEnabled(false);
        ui->arkistoNappi->setEnabled(false);
    }
}

void ArkistoSivu::teeArkisto(Tilikausi kausi)
{

    QProgressDialog odota(tr("Muodostetaan arkistoa"), QString(), 0, 100, this);
    odota.setMinimumDuration(250);

    QString sha = Arkistoija::arkistoi(kausi);
    kp()->tilikaudet()->merkitseArkistoiduksi( ui->view->currentIndex().row(), sha);    // Merkitsee arkistoinnin tehdyksi

    odota.setValue(100);

}
