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

#include "kirjauswg.h"
#include "tilidelegaatti.h"
#include "eurodelegaatti.h"
#include "pvmdelegaatti.h"

#include "db/kirjanpito.h"

#include <QDebug>
#include <QSqlQuery>
#include <QMessageBox>
#include <QIntValidator>
#include <QFileDialog>

#include <QSortFilterProxyModel>

KirjausWg::KirjausWg(TositeModel *tositeModel) : QWidget(), model(tositeModel)
{
    ui = new Ui::KirjausWg();
    ui->setupUi(this);

    ui->viennitView->setModel( model->vientiModel() );

    connect( model->vientiModel() , SIGNAL(siirryRuutuun(QModelIndex)), ui->viennitView, SLOT(setCurrentIndex(QModelIndex)));
    connect( model->vientiModel(), SIGNAL(siirryRuutuun(QModelIndex)), ui->viennitView, SLOT(edit(QModelIndex)));
    connect( model->vientiModel(), SIGNAL(muuttunut()), this, SLOT(naytaSummat()));

    ui->viennitView->setItemDelegateForColumn( VientiModel::PVM, new PvmDelegaatti(ui->tositePvmEdit));
    ui->viennitView->setItemDelegateForColumn( VientiModel::TILI, new TiliDelegaatti() );
    ui->viennitView->setItemDelegateForColumn( VientiModel::DEBET, new EuroDelegaatti);
    ui->viennitView->setItemDelegateForColumn( VientiModel::KREDIT, new EuroDelegaatti);

    ui->viennitView->hideColumn(VientiModel::PROJEKTI);
    ui->viennitView->hideColumn(VientiModel::KUSTANNUSPAIKKA);
    ui->viennitView->horizontalHeader()->setStretchLastSection(true);

    ui->tunnisteEdit->setValidator( new QIntValidator(1,99999999) );

    connect( ui->lisaaRiviNappi, SIGNAL(clicked(bool)), this, SLOT(lisaaRivi()));
    connect( ui->tallennaButton, SIGNAL(clicked(bool)), this, SLOT(tallenna()));
    connect( ui->hylkaaNappi, SIGNAL(clicked(bool)), this, SLOT(hylkaa()));
    connect( ui->kommentitEdit, SIGNAL(textChanged()), this, SLOT(paivitaKommenttiMerkki()));

    ui->tositetyyppiCombo->setModel( Kirjanpito::db()->tositelajit());
    ui->tositetyyppiCombo->setModelColumn( TositelajiModel::NIMI);

    connect( ui->tositePvmEdit, SIGNAL(dateChanged(QDate)), model, SLOT(asetaPvm(QDate)));
    connect( ui->tositetyyppiCombo, SIGNAL(currentIndexChanged(int)), this, SLOT(vaihdaTositeTyyppi()));
    connect( ui->tunnisteEdit, SIGNAL(textChanged(QString)), this, SLOT(paivitaTunnisteVari()));

    // Tiliotteen tilivalintaan hyväksytään vain rahoitustilit
    QSortFilterProxyModel *proxy = new QSortFilterProxyModel(this);
    proxy->setSourceModel( kp()->tilit());
    proxy->setFilterRole( TiliModel::TyyppiRooli);
    proxy->setFilterFixedString("AR");
    proxy->setSortRole(TiliModel::NroRooli);

    ui->tiliotetiliCombo->setModel( proxy );
    ui->tiliotetiliCombo->setModelColumn(TiliModel::NRONIMI);

    ui->liiteView->setModel( model->liiteModel() );
    connect( ui->lisaaliiteNappi, SIGNAL(clicked(bool)), this, SLOT(lisaaLiite()));
}

KirjausWg::~KirjausWg()
{
    delete ui;
    delete model;
}

QDate KirjausWg::tositePvm() const
{
    return ui->tositePvmEdit->date();
}

void KirjausWg::lisaaRivi()
{
    model->vientiModel()->lisaaRivi();
    ui->viennitView->setFocus();

    QModelIndex indeksi = model->vientiModel()->index( model->vientiModel()->rowCount(QModelIndex()) - 1, VientiModel::TILI );

    ui->viennitView->setCurrentIndex( indeksi  );
    ui->viennitView->edit( indeksi );
}

void KirjausWg::tyhjenna()
{
    // Tyhjennetään ensin model
    model->tyhjaa();
    // ja sitten päivitetään lomakkeen tiedot modelista
    tiedotModelista();
}

void KirjausWg::tallenna()
{
    tiedotModeliin();
    model->tallenna();
}

void KirjausWg::hylkaa()
{
    tyhjenna();
    ui->tositetyyppiCombo->setCurrentIndex(0);
    emit Kirjanpito::db()->palaaEdelliselleSivulle();
}

void KirjausWg::naytaSummat()
{
    // ui->summaLabel->setText( tr("Debet %L1 €    Kredit %L2 €").arg(((double)viennitModel->debetSumma())/100.0 ,0,'f',2)
    //                         .arg(((double)viennitModel->kreditSumma()) / 100.0 ,0,'f',2));
}

void KirjausWg::lataaTosite(int id)
{
    model->lataa(id);

    tiedotModelista();

    ui->tabWidget->setCurrentIndex(0);
    ui->tositePvmEdit->setFocus();

}

void KirjausWg::paivitaKommenttiMerkki()
{
    if( ui->kommentitEdit->document()->toPlainText().isEmpty())
    {
        ui->tabWidget->setTabIcon(1, QIcon());
    }
    else
    {
        ui->tabWidget->setTabIcon(1, QIcon(":/pic/kommentti.png"));
    }

}

void KirjausWg::paivitaTunnisteVari()
{
    if( model->kelpaakoTunniste( ui->tunnisteEdit->text().toInt() ))
        ui->tunnisteEdit->setStyleSheet("color: black;");
    else
        ui->tunnisteEdit->setStyleSheet("color: red;");

}

void KirjausWg::lisaaLiite()
{
    QString polku = QFileDialog::getOpenFileName(this, tr("Lisää liite"),QString(),tr("Kuvat (*.png *.jpg)"));
    if( !polku.isEmpty())
    {
        QFileInfo info(polku);
        model->liiteModel()->lisaaTiedosto(polku, info.fileName());
    }

}

void KirjausWg::tiedotModeliin()
{
    model->asetaPvm( ui->tositePvmEdit->date());
    model->asetaOtsikko( ui->otsikkoEdit->text());
    model->asetaTunniste( ui->tunnisteEdit->text().toInt());
    model->asetaTositelaji( ui->tositetyyppiCombo->currentData( TositelajiModel::IdRooli).toInt() );

    model->asetaKommentti( ui->kommentitEdit->toPlainText() );

    if( ui->tilioteBox->isChecked())
    {
        model->asetaTiliotetili( ui->tiliotetiliCombo->currentData(TiliModel::IdRooli).toInt() );
        model->json()->set("TilioteAlkaa", ui->tiliotealkaenEdit->date());
        model->json()->set("TilioteLoppuu", ui->tilioteloppuenEdit->date());
    }
    else
    {
        model->asetaTiliotetili(0);
        model->json()->unset("TilioteAlkaa");
        model->json()->unset("TilioteLoppuu");
    }
}

void KirjausWg::tiedotModelista()
{
    ui->tositePvmEdit->setDate( model->pvm() );
    ui->otsikkoEdit->setText( model->otsikko() );
    ui->kommentitEdit->setPlainText( model->kommentti());
    ui->tunnisteEdit->setText( QString::number(model->tunniste()));
    ui->tositetyyppiCombo->setCurrentIndex( ui->tositetyyppiCombo->findData( model->tositelaji().id(), TositelajiModel::IdRooli ) );

    ui->tilioteBox->setChecked( model->tiliotetili() != 0 );
    // Tiliotetilin yhdistämiset!
    if( model->tiliotetili())
    {
        ui->tiliotetiliCombo->setCurrentIndex( ui->tiliotetiliCombo->findData( QVariant(model->tiliotetili()) ,TiliModel::IdRooli ) );
        ui->tiliotealkaenEdit->setDate( model->json()->date("TilioteAlkaa") );
        ui->tilioteloppuenEdit->setDate( model->json()->date("TilioteLoppuu"));
    }
}

void KirjausWg::salliMuokkaus(bool sallitaanko)
{
    ui->tositePvmEdit->setEnabled(sallitaanko);
    ui->tositetyyppiCombo->setEnabled(sallitaanko);
    ui->kommentitEdit->setEnabled(sallitaanko);
    ui->tunnisteEdit->setEnabled(sallitaanko);
    ui->tallennaButton->setEnabled(sallitaanko);
    ui->otsikkoEdit->setEnabled(sallitaanko);

    // viennitModel->salliMuokkaus(sallitaanko);

    if(sallitaanko)
        ui->tositePvmEdit->setDateRange(Kirjanpito::db()->tilitpaatetty().addDays(1), kp()->tilikaudet()->kirjanpitoLoppuu() );
    else
        ui->tositePvmEdit->setDateRange( Kirjanpito::db()->tilikaudet()->kirjanpitoAlkaa() , Kirjanpito::db()->tilikaudet()->kirjanpitoLoppuu() );

}

void KirjausWg::vaihdaTositeTyyppi()
{
    model->asetaTositelaji( ui->tositetyyppiCombo->currentData(TositelajiModel::IdRooli).toInt() );
    ui->tyyppiLabel->setText( model->tositelaji().tunnus());

    // Päivitetään tositenumero modelista ;)
    ui->tunnisteEdit->setText( QString::number(model->tunniste() ));
}

