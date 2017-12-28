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


#include "arkistosivu.h"
#include "db/kirjanpito.h"
#include "ui_lisaatilikausidlg.h"
#include "ui_lukitsetilikausi.h"
#include "ui_muokkaatilikausi.h"

#include "arkistoija/arkistoija.h"
#include "tilinpaatoseditori/tilinpaatoseditori.h"
#include "tilinpaatoseditori/tpaloitus.h"

#include "tilinpaattaja.h"


ArkistoSivu::ArkistoSivu()
{
    ui = new Ui::TilikausiMaaritykset;
    ui->setupUi(this);

    connect( ui->uusiNappi, SIGNAL(clicked(bool)), this, SLOT(uusiTilikausi()));
    connect( ui->arkistoNappi, SIGNAL(clicked(bool)), this, SLOT(arkisto()));
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

        QDesktopServices::openUrl( QUrl::fromLocalFile( kp()->hakemisto().absoluteFilePath("arkisto/" + kausi.arkistoHakemistoNimi()) + "/index.html" ));

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
            QDesktopServices::openUrl( QUrl::fromLocalFile( kp()->hakemisto().absoluteFilePath("arkisto/" + kausi.arkistoHakemistoNimi()) + "/tilinpaatos.pdf" ));
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
        ui->arkistoNappi->setEnabled( kausi.paattyy() > kp()->tilitpaatetty() || kausi.arkistoitu().isValid());

        // Muokata voidaan vain viimeistä tilikautta, jos tilinpäätös ei ole valmis
        ui->muokkaaNappi->setEnabled( kausi.paattyy() == kp()->tilikaudet()->kirjanpitoLoppuu()  &&
                                      kausi.tilinpaatoksenTila() != Tilikausi::VAHVISTETTU );
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
    QDateTime nyt = QDateTime( kp()->paivamaara(), QTime::currentTime());
    kp()->tilikaudet()->json(kausi)->set("Arkistoitu", nyt.toString(Qt::ISODate) );
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

    Tilikausi kausi = kp()->tilikaudet()->tilikausiIndeksilla( kp()->tilikaudet()->rowCount(QModelIndex()) - 1 );

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
    dlgUi.poistaRadio->setEnabled( viimepaiva < kausi.alkaa() );

    if( viimepaiva < kausi.alkaa() )
        viimepaiva = kausi.alkaa().addDays(1);

    dlgUi.paattyyDate->setDateRange( viimepaiva, kausi.alkaa().addMonths(19).addDays(-1) );
    dlgUi.paattyyDate->setDate( kausi.paattyy() );

    if( dlg.exec())
    {
        if( dlgUi.poistaRadio->isChecked())
            kp()->tilikaudet()->muokkaaViimeinenTilikausi( QDate());
        else
            kp()->tilikaudet()->muokkaaViimeinenTilikausi( dlgUi.paattyyDate->date() );
    }

}
