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
#include <QSettings>
#include <QDesktopServices>
#include <QUrl>
#include <QProgressDialog>
#include <QSqlQuery>
#include <QMessageBox>
#include <QFileDialog>
#include <QDirIterator>
#include <QDebug>

#include <fstream>
#include <iostream>

#include "arkistosivu.h"
#include "db/kirjanpito.h"
#include "uusitilikausidlg.h"

#include "ui_lukitsetilikausi.h"

#include "ui_arkistonvienti.h"

#include "arkistoija/arkistoija.h"
#include "tilinpaatoseditori/tilinpaatoseditori.h"
#include "tilinpaatoseditori/tpaloitus.h"

#include "tilinpaattaja.h"

#include "tararkisto.h"
#include "naytin/naytinikkuna.h"

#include "budjettidlg.h"
#include "db/yhteysmodel.h"

#include "uudelleennumerointi.h"
#include "tilikausimuokkausdlg.h"

#include "arkistoija/aineistodialog.h"

#include <zip.h>

ArkistoSivu::ArkistoSivu()
{
    ui = new Ui::TilikausiMaaritykset;
    ui->setupUi(this);

    ui->view->setModel( kp()->tilikaudet() );
    ui->view->hideColumn(TilikausiModel::LYHENNE);

    connect( ui->uusiNappi, SIGNAL(clicked(bool)), this, SLOT(uusiTilikausi()));
    connect( ui->aineistoNappi, &QPushButton::clicked, this, &ArkistoSivu::aineisto);
    connect( ui->arkistoNappi, SIGNAL(clicked(bool)), this, SLOT(arkisto()));
    connect( ui->vieNappi, SIGNAL(clicked(bool)), this, SLOT(vieArkisto()));
    connect( ui->tilinpaatosNappi, SIGNAL(clicked(bool)), this, SLOT(tilinpaatos()));
    connect( ui->muokkaaNappi, SIGNAL(clicked(bool)), this, SLOT(muokkaa()));
    connect( ui->budjettiNappi, &QPushButton::clicked, this, &ArkistoSivu::budjetti);
    connect( ui->numeroiButton, &QPushButton::clicked, this, &ArkistoSivu::uudellenNumerointi);
    connect( kp()->tilikaudet(), &TilikausiModel::modelReset, [this] {  if(this->ui->view->model()) this->ui->view->selectRow( ui->view->model()->rowCount()-1 );});

    connect( ui->view->selectionModel(), &QItemSelectionModel::selectionChanged, this, &ArkistoSivu::nykyinenVaihtuuPaivitaNapit );    
}

ArkistoSivu::~ArkistoSivu()
{    
    delete ui;
}

void ArkistoSivu::siirrySivulle()
{
    if( !kp()->yhteysModel())
        return;

    kp()->tilikaudet()->paivita();
    ui->view->resizeColumnsToContents();

    for(int i=0; i < 6; i++)
        ui->view->setColumnWidth( i, (ui->view->width()-10) / 6);
    ui->view->horizontalHeader()->setStretchLastSection(true);

    // Pyritään valitsemaan tilikausi, jolle laaditaan seuraavaksi tilinpäätös
    ui->view->selectRow( ui->view->model()->rowCount(QModelIndex()) - 1);
    for(int i=0; i < kp()->tilikaudet()->rowCount(QModelIndex()); i++) {
        if( kp()->tilikaudet()->tilikausiIndeksilla(i).tilinpaatoksenTila() < Tilikausi::VAHVISTETTU) {
            ui->view->selectRow(i);
            break;
        }
    }
    ui->uusiNappi->setVisible( kp()->yhteysModel()->onkoOikeutta(YhteysModel::ASETUKSET) );
    ui->muokkaaNappi->setVisible( kp()->yhteysModel()->onkoOikeutta(YhteysModel::ASETUKSET) );
    ui->tilinpaatosNappi->setVisible( kp()->yhteysModel()->onkoOikeutta(YhteysModel::TILINPAATOS));
    ui->budjettiNappi->setVisible( kp()->yhteysModel()->onkoOikeutta(YhteysModel::BUDJETTI));
    ui->numeroiButton->setVisible( kp()->yhteysModel()->onkoOikeutta(YhteysModel::ASETUKSET));
}

void ArkistoSivu::uusiTilikausi()
{
    UusiTilikausiDlg dlg;
    dlg.exec();
}

void ArkistoSivu::aineisto()
{
    if( ui->view->currentIndex().isValid())
    {
        AineistoDialog *aineisto = new AineistoDialog();
        Tilikausi kausi = kp()->tilikaudet()->tilikausiIndeksilla( ui->view->currentIndex().row() );        
        aineisto->aineisto(kausi.alkaa());
    }
}

void ArkistoSivu::arkisto()
{
    if( ui->view->currentIndex().isValid())
    {
        Tilikausi kausi = kp()->tilikaudet()->tilikausiIndeksilla( ui->view->currentIndex().row() );
        QString arkistotiedosto = kausi.uusiArkistopolku() + "/index.html";

        // Tehdään arkisto, jos se on päivittämisen tarpeessa
        if( !kausi.arkistoitu().isValid() || kausi.arkistoitu() < kausi.viimeinenPaivitys() || !QFile::exists(arkistotiedosto))
        {
            Arkistoija* arkistoija = new Arkistoija(kausi, this);
            connect( arkistoija, &Arkistoija::arkistoValmis, [] (const QString& polku) { Kirjanpito::avaaUrl( QUrl::fromLocalFile( polku + "/index.html" )); } );
            arkistoija->arkistoi();
        } else {
            Kirjanpito::avaaUrl( QUrl::fromLocalFile(  arkistotiedosto ));
        }

    }
}

void ArkistoSivu::vieArkisto()
{
    if( !ui->view->currentIndex().isValid())
        return;

    Tilikausi kausi = kp()->tilikaudet()->tilikausiIndeksilla( ui->view->currentIndex().row() );
    QString arkistotiedosto = kausi.uusiArkistopolku() + "/index.html";


    // Tehdään arkisto, jos se on päivittämisen tarpeessa
    if( !kausi.arkistoitu().isValid() || kausi.arkistoitu() < kausi.viimeinenPaivitys() || !QFile::exists(arkistotiedosto))
    {
        Arkistoija* arkistoija = new Arkistoija(kausi, this);
        connect( arkistoija, &Arkistoija::arkistoValmis, this, &ArkistoSivu::jatkaVientia );
        arkistoija->arkistoi();
    } else {
        jatkaVientia( kausi.arkistopolku() );
    }

}

void ArkistoSivu::jatkaVientia(const QString& polku)
{
    QDialog dlg;
    Ui::ArkistonVienti dlgUi;
    dlgUi.setupUi(&dlg);

    if(dlg.exec() == QDialog::Rejected)
        return;
    if( dlgUi.hakemistoRadio->isChecked())
    {
        vieHakemistoon( polku );
    }
    else if( dlgUi.zipButton->isChecked())
    {
        if( !teeZip(polku))
            QMessageBox::critical(this, tr("Arkiston viennissä virhe"),
                                  tr("Arkiston vienti epäonnistui.") );
    }
}

void ArkistoSivu::tilinpaatos()
{
    // Uusi tilinpäättäjään perustuva
    if( ui->view->currentIndex().isValid())
    {
        Tilikausi kausi = kp()->tilikaudet()->tilikausiIndeksilla( ui->view->currentIndex().row() );

        if( kausi.tilinpaatoksenTila() == Tilikausi::VAHVISTETTU && kp()->tilitpaatetty() >= kausi.paattyy() )
        {        
            NaytinIkkuna::naytaLiite(0, QString("TP_%1").arg(kausi.paattyy().toString(Qt::ISODate)) );
        }
        else
        {
            TilinPaattaja *paattaja = new TilinPaattaja(kausi, this, parentWidget() );
            connect( paattaja, SIGNAL(lukittu(Tilikausi)), this, SLOT(teeArkisto(Tilikausi)));
            paattaja->show();
        }
    }

}

void ArkistoSivu::tilinpaatosKasky()
{
    ui->view->setCurrentIndex( ui->view->model()->index( kp()->tilikaudet()->indeksiPaivalle( kp()->paivamaara().addMonths(-4) ),0) );
    tilinpaatos();
}

void ArkistoSivu::nykyinenVaihtuuPaivitaNapit()
{
    int indeksi = ui->view->selectionModel()->selectedRows().value(0).row();
    qDebug() << " indekkksi "  << indeksi;

    if( indeksi > -1)
    {
        Tilikausi kausi = kp()->tilikaudet()->tilikausiIndeksilla( indeksi );        
        // Tilikaudelle voi tehdä tilinpäätöksen, jos se ei ole tilinavaus
        bool paatettava = kausi.tilinpaatoksenTila() != Tilikausi::EILAADITATILINAVAUKSELLE && ( kausi.tase() || kausi.viimeinenPaivitys().isValid() );
        ui->tilinpaatosNappi->setEnabled( paatettava );

        ui->arkistoNappi->setEnabled( paatettava );
        ui->vieNappi->setEnabled( paatettava );
        ui->aineistoNappi->setEnabled( paatettava );
        ui->numeroiButton->setEnabled( kausi.alkaa() > kp()->tilitpaatetty() );
        ui->muokkaaNappi->setEnabled( true );
    }
    else
    {
        ui->tilinpaatosNappi->setEnabled(false);
        ui->arkistoNappi->setEnabled(false);
        ui->muokkaaNappi->setEnabled(false);
        ui->aineistoNappi->setEnabled(false);
        ui->numeroiButton->setEnabled(false);
    }
}

void ArkistoSivu::teeArkisto(Tilikausi kausi)
{
    Arkistoija* arkistoija = new Arkistoija(kausi, this);
    arkistoija->arkistoi();
}

void ArkistoSivu::muokkaa()
{
    Tilikausi kausi = kp()->tilikaudet()->tilikausiIndeksilla(ui->view->currentIndex().row() );

    TilikausiMuokkausDlg *muokkaus = new TilikausiMuokkausDlg(this);
    muokkaus->muokkaa(kausi);

    return;
}

void ArkistoSivu::budjetti()
{
    BudjettiDlg *dlg = new BudjettiDlg(this);
    dlg->setAttribute(Qt::WA_DeleteOnClose);
    dlg->show();
    dlg->lataa( ui->view->currentIndex().data(TilikausiModel::AlkaaRooli).toDate() );
}

void ArkistoSivu::uudellenNumerointi()
{
    if( ui->view->currentIndex().isValid())
    {
        Tilikausi kausi = kp()->tilikaudet()->tilikausiIndeksilla( ui->view->currentIndex().row() );
        Uudelleennumerointi::numeroiUudelleen(kausi);
    }
}

bool ArkistoSivu::teeZip(const QString &polku)
{
    QDirIterator iter( polku,
                       QDir::Files,
                       QDirIterator::Subdirectories);
    QStringList tiedostot;

    while( iter.hasNext()) {
        tiedostot.append( iter.next());
    }


      QDir mista( polku );


    QString arkistotiedosto = QDir::root().absoluteFilePath( QString("%1.zip").arg(polku) );
    QString arkisto = QFileDialog::getSaveFileName(this, tr("Vie arkisto"), arkistotiedosto, tr("Zip-arkisto (*.zip)") );


    if( !arkisto.isEmpty() )
    {
        int virhekoodi = 0;
        zip_t* paketti = zip_open( arkisto.toStdString().c_str(), ZIP_CREATE | ZIP_TRUNCATE, &virhekoodi );

        if( !paketti)
            return false;

        QProgressDialog odota(tr("Kopioidaan arkistoa"), tr("Peruuta"),0, tiedostot.count(),this);
        int kopioitu = 0;

        for( const QString& tiedosto : tiedostot)
        {
            if( odota.wasCanceled())
                return false;

            QFileInfo info( mista.absoluteFilePath(tiedosto)) ;

            zip_source_t* lahde = zip_source_file(paketti, info.absoluteFilePath().toStdString().c_str(),
                                                  0,-1);
            if( !lahde)
                return false;
            if( zip_file_add(paketti, mista.relativeFilePath(tiedosto).toStdString().c_str(),
                             lahde, 0) < 0)
                return false;

            odota.setValue(++kopioitu);
        }
        zip_close(paketti);

        QMessageBox::information(this, tr("Arkiston vienti valmis"),
                             tr("Arkisto viety tiedostoon %1").arg(arkisto));
    }
    return true;
}

bool ArkistoSivu::vieHakemistoon(const QString &polku)
{
    QDir mista( polku );

    QDirIterator iter( mista.path(),
                       QDir::Files,
                       QDirIterator::Subdirectories);
    QStringList tiedostot;

    while( iter.hasNext()) {
        iter.next();
        tiedostot.append(  mista.relativeFilePath(iter.filePath()) );
    }

    QString hakemistoon = QFileDialog::getExistingDirectory(this, tr("Valitse hakemisto, jonne arkisto kopioidaan"),
                                                            QDir::rootPath());
    if( !hakemistoon.isEmpty())
    {

        QDir minne( hakemistoon );
        minne.mkdir("liitteet");
        minne.mkdir("static");
        minne.mkdir("tositteet");



        QProgressDialog odota(tr("Kopioidaan arkistoa"), tr("Peruuta"),0, tiedostot.count(),this);
        int kopioitu = 0;
        for( const QString& tiedosto : tiedostot)
        {
            if( odota.wasCanceled())
                break;
            if( !QFile::copy( mista.absoluteFilePath(tiedosto), minne.absoluteFilePath(tiedosto) ) )
            {
                QMessageBox::critical(this, tr("Kopiointi ei onnistu"), tr("Tiedoston %1 kopiointi ei onnistunut.\n Kopiointi on keskeytetty.").arg(tiedosto) );
                return false;
            }
            odota.setValue(++kopioitu);
        }
    }
    QMessageBox::information(this, tr("Arkiston kopiointi valmis"), tr("Arkisto on kopioitu hakemistoon %1.\n"
                                                                       "Avaa selaimella hakemistossa oleva index.html-tiedosto.").arg(hakemistoon));
    return true;
}
