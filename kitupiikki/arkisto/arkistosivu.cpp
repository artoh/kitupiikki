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
#include <QSqlQuery>
#include <QMessageBox>
#include <QFileDialog>

#include <fstream>
#include <iostream>

#include "arkistosivu.h"
#include "db/kirjanpito.h"
#include "ui_lisaatilikausidlg.h"
#include "ui_lukitsetilikausi.h"
#include "ui_muokkaatilikausi.h"

#include "ui_arkistonvienti.h"

#include "arkistoija/arkistoija.h"
#include "tilinpaatoseditori/tilinpaatoseditori.h"
#include "tilinpaatoseditori/tpaloitus.h"

#include "tilinpaattaja.h"

#include "tararkisto.h"


ArkistoSivu::ArkistoSivu()
{
    ui = new Ui::TilikausiMaaritykset;
    ui->setupUi(this);

    connect( ui->uusiNappi, SIGNAL(clicked(bool)), this, SLOT(uusiTilikausi()));
    connect( ui->arkistoNappi, SIGNAL(clicked(bool)), this, SLOT(arkisto()));
    connect( ui->vieNappi, SIGNAL(clicked(bool)), this, SLOT(vieArkisto()));
    connect( ui->tilinpaatosNappi, SIGNAL(clicked(bool)), this, SLOT(tilinpaatos()));
    connect( ui->muokkaaNappi, SIGNAL(clicked(bool)), this, SLOT(muokkaa()));

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
        kp()->tilikaudet()->tallennaJSON();
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

        Kirjanpito::avaaUrl( QUrl::fromLocalFile( kp()->hakemisto().absoluteFilePath("arkisto/" + kausi.arkistoHakemistoNimi()) + "/index.html" ));

    }
}

void ArkistoSivu::vieArkisto()
{
    if( !ui->view->currentIndex().isValid())
        return;

    QDialog dlg;
    Ui::ArkistonVienti dlgUi;
    dlgUi.setupUi(&dlg);

    if(dlg.exec() == QDialog::Rejected)
        return;

    Tilikausi kausi = kp()->tilikaudet()->tilikausiIndeksilla( ui->view->currentIndex().row() );

    // Tehdään arkisto, jos se on päivittämisen tarpeessa
    if( !kausi.arkistoitu().isValid() || kausi.arkistoitu() < kausi.viimeinenPaivitys())
    {
        teeArkisto(kausi);
    }

    QDir mista( kp()->hakemisto().absoluteFilePath("arkisto/" + kausi.arkistoHakemistoNimi() ) );
    QStringList tiedostot = mista.entryList(QDir::Files);



    if( dlgUi.hakemistoRadio->isChecked())
    {
        QString hakemistoon = QFileDialog::getExistingDirectory(this, tr("Valitse hakemisto, jonne arkisto kopioidaan"),
                                                                QDir::rootPath());
        if( !hakemistoon.isEmpty())
        {
            QDir minne(hakemistoon);

            QProgressDialog odota(tr("Kopioidaan arkistoa"), tr("Peruuta"),0, tiedostot.count(),this);
            int kopioitu = 0;
            for( QString tiedosto : tiedostot)
            {
                if( odota.wasCanceled())
                    break;
                if( !QFile::copy( mista.absoluteFilePath(tiedosto), minne.absoluteFilePath(tiedosto) ) )
                {
                    QMessageBox::critical(this, tr("Kopiointi ei onnistu"), tr("Tiedoston %1 kopiointi ei onnistunut.\n Kopiointi on keskeytetty.").arg(tiedosto));
                    return;
                }
                odota.setValue(++kopioitu);
            }
        }
        QMessageBox::information(this, tr("Arkiston kopiointi valmis"), tr("Arkisto on kopioitu hakemistoon %1.\n"
                                                                           "Avaa selaimella hakemistossa oleva index.html-tiedosto.").arg(hakemistoon));

    }
    else
    {
        // Muodostetaan tar-arkisto
        // Tässä käytetään Pierre Lindenbaumin Tar-luokkaa http://plindenbaum.blogspot.fi/2010/08/creating-tar-file-in-c.html

        QString arkistotiedosto = QDir::root().absoluteFilePath( QString("%1-%2.tar").arg( kp()->hakemisto().dirName() ).arg(kausi.arkistoHakemistoNimi()) );
        QString arkisto = QFileDialog::getSaveFileName(this, tr("Vie arkisto"), arkistotiedosto, tr("Tar-arkisto (*.tar)") );


        if( !arkisto.isEmpty() )
        {
            TarArkisto tar( arkisto );
            if( !tar.aloita())
            {
                QMessageBox::critical(this, tr("Arkiston vienti epäonnistui"),
                                      tr("Tiedostoon %1 kirjoittaminen epäonnistui").arg(arkisto));
                return;
            }

            QProgressDialog odota(tr("Kopioidaan arkistoa"), tr("Peruuta"),0, tiedostot.count(),this);
            int kopioitu = 0;

            for( QString tiedosto : tiedostot)
            {
                if( odota.wasCanceled())
                    break;

                if( !tar.lisaaTiedosto( mista.absoluteFilePath(tiedosto) ) )
                {
                    tar.lopeta();

                    QFile::remove( arkisto );

                    QMessageBox::critical(this, tr("Arkiston viennissä virhe"),
                                          tr("Arkiston vienti epäonnistui kopioitaessa tiedostoa %1")
                                          .arg(tiedosto));
                    return;
                }

                odota.setValue(++kopioitu);
            }

            QMessageBox::information(this, tr("Arkisto vienti valmis"),
                                 tr("Arkisto viety tiedostoon %1").arg(arkisto));

        }




    }



}

void ArkistoSivu::tilinpaatos()
{
    // Uusi tilinpäättäjään perustuva
    if( ui->view->currentIndex().isValid())
    {
        Tilikausi kausi = kp()->tilikaudet()->tilikausiIndeksilla( ui->view->currentIndex().row() );

        if( kausi.tilinpaatoksenTila() == Tilikausi::VAHVISTETTU )
        {
            // Avataan tilinpäätös
            Kirjanpito::avaaUrl( QUrl::fromLocalFile( kp()->hakemisto().absoluteFilePath("arkisto/" + kausi.arkistoHakemistoNimi()) + "/tilinpaatos.pdf" ));
        }
        else
        {
            TilinPaattaja *paattaja = new TilinPaattaja(kausi, parentWidget() );
            connect( paattaja, SIGNAL(lukittu(Tilikausi)), this, SLOT(teeArkisto(Tilikausi)));
            paattaja->show();
        }
    }
    return;

}

void ArkistoSivu::nykyinenVaihtuuPaivitaNapit()
{
    if( ui->view->currentIndex().isValid())
    {
        Tilikausi kausi = kp()->tilikaudet()->tilikausiIndeksilla( ui->view->currentIndex().row() );
        // Tilikaudelle voi tehdä tilinpäätöksen, jos se ei ole tilinavaus
        ui->tilinpaatosNappi->setEnabled( kausi.tilinpaatoksenTila() != Tilikausi::EILAADITATILINAVAUKSELLE );
        // Tilikauden voi arkistoida, jos tilikautta ei ole lukittu - arkiston voi näyttää aina
        bool arkistoitavissa = ( kausi.paattyy() > kp()->tilitpaatetty() || kausi.arkistoitu().isValid() )
                && kausi.tilinpaatoksenTila() != Tilikausi::EILAADITATILINAVAUKSELLE;


        ui->arkistoNappi->setEnabled( arkistoitavissa );
        ui->vieNappi->setEnabled( arkistoitavissa );


        // Muokata voidaan vain viimeistä tilikautta tai poistaa lukitus
        ui->muokkaaNappi->setEnabled( kausi.paattyy() == kp()->tilikaudet()->kirjanpitoLoppuu()  ||
                                     kp()->tilitpaatetty() >= kausi.alkaa() );
    }
    else
    {
        ui->tilinpaatosNappi->setEnabled(false);
        ui->arkistoNappi->setEnabled(false);
        ui->muokkaaNappi->setEnabled(false);
    }
}

void ArkistoSivu::teeArkisto(Tilikausi kausi)
{

    QProgressDialog odota(tr("Muodostetaan arkistoa"), QString(), 0, 100, this);
    odota.setMinimumDuration(250);

    QString sha = Arkistoija::arkistoi(kausi);

    // Merkitsee arkistoiduksi

    kp()->tilikaudet()->json(kausi)->set("Arkistoitu", QDateTime::currentDateTime().toString(Qt::ISODate) );
    kp()->tilikaudet()->json(kausi)->set("ArkistoSHA", sha);
    kp()->tilikaudet()->tallennaJSON();

    QModelIndex indeksi = kp()->tilikaudet()->index( kp()->tilikaudet()->indeksiPaivalle(kausi.paattyy()) , TilikausiModel::ARKISTOITU );
    emit kp()->tilikaudet()->dataChanged( indeksi, indeksi );

    odota.setValue(100);

}

void ArkistoSivu::muokkaa()
{
    QDialog dlg;
    Ui::MuokkaaTilikausi dlgUi;
    dlgUi.setupUi(&dlg);

    Tilikausi kausi = kp()->tilikaudet()->tilikausiIndeksilla(ui->view->currentIndex().row() );

    // Selvitetään, mikä on viimeisin kirjaus
    QDate viimepaiva = kp()->tilitpaatetty();
    QSqlQuery kysely("SELECT MAX(pvm) from vienti");
    if( kysely.next())
    {
        QDate viimevienti = kysely.value(0).toDate();
        if( viimevienti > viimepaiva)
            viimepaiva = viimevienti;
    }

    kysely.exec("SELECT MAX(pvm) from tosite");
    if( kysely.next())
    {
        QDate viimevienti = kysely.value(0).toDate();
        if( viimevienti > viimepaiva)
            viimepaiva = viimevienti;
    }

    // Saa poistaa vain, ellei yhtään kirjausta
    dlgUi.poistaRadio->setEnabled( viimepaiva < kausi.alkaa() && kausi.paattyy() == kp()->tilikaudet()->kirjanpitoLoppuu() );
    dlgUi.paattyyRadio->setEnabled( kausi.paattyy() == kp()->tilikaudet()->kirjanpitoLoppuu() );
    dlgUi.peruLukko->setEnabled( kp()->tilitpaatetty() >= kausi.alkaa() );
    dlgUi.peruLukko->setChecked( kausi.paattyy() != kp()->tilikaudet()->kirjanpitoLoppuu() );

    if( viimepaiva < kausi.alkaa() )
        viimepaiva = kausi.alkaa().addDays(1);

    dlgUi.paattyyDate->setDateRange( viimepaiva, kausi.alkaa().addMonths(19).addDays(-1) );
    dlgUi.paattyyDate->setDate( kausi.paattyy() );

    if( dlg.exec())
    {
        if( dlgUi.poistaRadio->isChecked())
            kp()->tilikaudet()->muokkaaViimeinenTilikausi( QDate());
        else if( dlgUi.paattyyRadio->isChecked())
            kp()->tilikaudet()->muokkaaViimeinenTilikausi( dlgUi.paattyyDate->date() );
        else if( dlgUi.peruLukko->isChecked())
        {
            if( QMessageBox::warning(this, tr("Tilikauden lukitsemisen peruminen"),
                                     tr("Oletko varma, että haluat perua tilikauden lukitsemisen?\n\n"
                                        "Kaikki tilinpäätökseen liittyvät toimet on tehtävä uudelleen ja tilinpäätös on mahdollisesti "
                                        "myös vahvistettava uudelleen.\n\n"
                                        "Kirjanpitolaki 2. luku 7§ 2 mom:\n"
                                        "Tositteen, kirjanpidon tai muun kirjanpitoaineiston sisältöä ei saa muuttaa tai poistaa "
                                        "tilinpäätöksen laatimisen jälkeen."), QMessageBox::Yes | QMessageBox::Cancel, QMessageBox::Cancel ) != QMessageBox::Yes)
                return;

            JsonKentta *json = kp()->tilikaudet()->json( kausi );
            json->unset("Vahvistettu");
            kp()->tilikaudet()->tallennaJSON();
            kp()->asetukset()->aseta("TilitPaatetty", kausi.alkaa().addDays(-1));

        }
    }

}
