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

#include "tilikausimaaritykset.h"
#include "db/kirjanpito.h"
#include "ui_lisaatilikausidlg.h"

#include "arkistoija/arkistoija.h"


TilikausiMaaritykset::TilikausiMaaritykset()
{
    ui = new Ui::TilikausiMaaritykset;
    ui->setupUi(this);

    connect( ui->uusiNappi, SIGNAL(clicked(bool)), this, SLOT(uusiTilikausi()));
    connect( ui->arkistoNappi, SIGNAL(clicked(bool)), this, SLOT(arkisto()));

}

TilikausiMaaritykset::~TilikausiMaaritykset()
{
    delete ui;
}

bool TilikausiMaaritykset::nollaa()
{
    ui->view->setModel( kp()->tilikaudet() );
    ui->view->resizeColumnsToContents();

    connect( ui->view->selectionModel() , SIGNAL(currentRowChanged(QModelIndex,QModelIndex)), this, SLOT(nykyinenVaihtuuPaivitaNapit()) );

    ui->view->selectRow( ui->view->model()->rowCount(QModelIndex()) - 1);


    return true;
}

void TilikausiMaaritykset::uusiTilikausi()
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

void TilikausiMaaritykset::arkisto()
{
    if( ui->view->currentIndex().isValid())
    {
        Tilikausi kausi = kp()->tilikaudet()->tilikausiIndeksilla( ui->view->currentIndex().row() );

        // Tehdään arkisto, jos se on päivittämisen tarpeessa
        if( !kausi.arkistoitu().isValid() || kausi.arkistoitu() < kausi.viimeinenPaivitys())
        {
            QProgressDialog odota(tr("Muodostetaan arkistoa"), QString(), 0, 100, this);
            odota.setMinimumDuration(250);

            QString sha = Arkistoija::arkistoi(kausi);
            kp()->tilikaudet()->merkitseArkistoiduksi( ui->view->currentIndex().row(), sha);    // Merkitsee arkistoinnin tehdyksi
            kp()->tilikaudet()->tallenna();

            odota.setValue(100);
        }
        // Avataan arkistoi

        QDesktopServices::openUrl( QUrl::fromLocalFile( kp()->hakemisto().absoluteFilePath("arkisto/" + kausi.arkistoHakemistoNimi()) + "/index.html" ));

    }
}

void TilikausiMaaritykset::nykyinenVaihtuuPaivitaNapit()
{
    if( ui->view->currentIndex().isValid())
    {
        Tilikausi kausi = kp()->tilikaudet()->tilikausiIndeksilla( ui->view->currentIndex().row() );
        // Tilikaudelle voi tehdä tilinpäätöksen, jos aika on jo ajanut ohi
        ui->tilinpaatosNappi->setEnabled( kausi.paattyy() <= kp()->paivamaara() );
        // Tilikauden voi arkistoida, jos tilikautta ei ole lukittu - arkiston voi näyttää aina
        ui->arkistoNappi->setEnabled( kausi.paattyy() < kp()->tilitpaatetty() || kausi.arkistoitu().isValid());
    }
    else
    {
        ui->tilinpaatosNappi->setEnabled(false);
        ui->arkistoNappi->setEnabled(false);
    }
}
