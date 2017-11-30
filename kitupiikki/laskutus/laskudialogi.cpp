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

#include <QPrinter>
#include <QDesktopServices>
#include <QTemporaryFile>


#include "db/kirjanpito.h"

#include "laskudialogi.h"
#include "ui_laskudialogi.h"
#include "laskuntulostaja.h"
#include "laskutusverodelegaatti.h"

#include "kirjaus/eurodelegaatti.h"
#include "kirjaus/kohdennusdelegaatti.h"
#include "kirjaus/tilidelegaatti.h"

#include "kirjaus/verodialogi.h"

LaskuDialogi::LaskuDialogi(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::LaskuDialogi)
{
    ui->setupUi(this);

    ui->perusteCombo->addItem("Suoriteperusteinen", LaskuModel::SUORITEPERUSTE);
    ui->perusteCombo->addItem("Laskutusperusteinen", LaskuModel::LASKUTUSPERUSTE);
    ui->perusteCombo->addItem("Maksuperusteinen", LaskuModel::MAKSUPERUSTE);
    ui->perusteCombo->addItem("Käteiskuitti", LaskuModel::KATEISLASKU);

    ui->toimitusDate->setMinimumDate( kp()->tilitpaatetty().addDays(1));
    ui->eraDate->setMinimumDate( kp()->paivamaara() );
    ui->perusteCombo->setCurrentIndex( ui->perusteCombo->findData( kp()->asetukset()->luku("LaskuKirjausperuste") ));
    perusteVaihtuu();

    ui->toimitusDate->setDate( kp()->paivamaara() );
    ui->eraDate->setDate( kp()->paivamaara().addDays( kp()->asetukset()->luku("LaskuMaksuaika")));

    model = new LaskuModel(this);
    model->lisaaRivi();

    ui->nroLabel->setText( QString::number(model->laskunro()));

    ui->rivitView->setModel(model);
    ui->rivitView->setItemDelegateForColumn(LaskuModel::AHINTA, new EuroDelegaatti());
    ui->rivitView->setItemDelegateForColumn(LaskuModel::TILI, new TiliDelegaatti());
    ui->rivitView->setItemDelegateForColumn(LaskuModel::KOHDENNUS, new KohdennusDelegaatti());
    ui->rivitView->setItemDelegateForColumn(LaskuModel::BRUTTOSUMMA, new EuroDelegaatti());
    ui->rivitView->setItemDelegateForColumn(LaskuModel::ALV, new LaskutusVeroDelegaatti());

    ui->rivitView->setColumnHidden( LaskuModel::ALV, !kp()->asetukset()->onko("AlvVelvollinen") );
    ui->rivitView->setColumnHidden( LaskuModel::KOHDENNUS, kp()->kohdennukset()->rowCount(QModelIndex()) < 2);

    connect( ui->lisaaNappi, SIGNAL(clicked(bool)), model, SLOT(lisaaRivi()));
    connect( ui->esikatseluNappi, SIGNAL(clicked(bool)), this, SLOT(esikatsele()));
    connect( model, SIGNAL(summaMuuttunut(int)), this, SLOT(paivitaSumma(int)));
    connect( ui->perusteCombo, SIGNAL(currentIndexChanged(int)), this, SLOT(perusteVaihtuu()));

    ui->rivitView->horizontalHeader()->setSectionResizeMode(LaskuModel::NIMIKE, QHeaderView::Stretch);

    tulostaja = new LaskunTulostaja(model);
}

LaskuDialogi::~LaskuDialogi()
{
    delete ui;
}

void LaskuDialogi::viewAktivoitu(QModelIndex indeksi)
{
    if( indeksi.column() == LaskuModel::ALV)
    {
        VeroDialogiValinta uusivero = VeroDialogi::veroDlg( indeksi.data(LaskuModel::AlvKoodiRooli).toInt(), indeksi.data(LaskuModel::AlvProsenttiRooli).toInt(), true );
        model->setData(indeksi, uusivero.verokoodi, LaskuModel::AlvKoodiRooli);
        model->setData(indeksi, uusivero.veroprosentti, LaskuModel::AlvProsenttiRooli);
    }
}

void LaskuDialogi::paivitaSumma(int summa)
{
    ui->summaLabel->setText( QString("%L1 €").arg(summa / 100.0,0,'f',2) );
}

void LaskuDialogi::esikatsele()
{
    vieMalliin();

    // Luo tilapäisen pdf-tiedoston
    QTemporaryFile *file = new QTemporaryFile(QDir::tempPath() + "/lasku-XXXXXX.pdf", this);
    file->open();
    file->close();

    QPrinter printer;
    printer.setPaperSize(QPrinter::A4);
    printer.setOutputFormat(QPrinter::PdfFormat);
    printer.setOutputFileName( file->fileName() );

    tulostaja->tulosta(&printer);

    QDesktopServices::openUrl( QUrl(file->fileName()) );
}

void LaskuDialogi::perusteVaihtuu()
{
    int peruste = ui->perusteCombo->currentData().toInt();

    ui->rahaTiliEdit->setVisible( peruste != LaskuModel::MAKSUPERUSTE );
    ui->rahatiliLabel->setVisible( peruste != LaskuModel::MAKSUPERUSTE );

    if( peruste == LaskuModel::SUORITEPERUSTE || peruste == LaskuModel::LASKUTUSPERUSTE )
    {
        ui->rahaTiliEdit->suodataTyypilla("AS");
        ui->rahaTiliEdit->valitseTiliNumerolla( kp()->asetus("LaskuSaatavatili").toInt());
    }
    else if( peruste == LaskuModel::KATEISLASKU)
    {
        ui->rahaTiliEdit->suodataTyypilla("AR");
        ui->rahaTiliEdit->valitseTiliNumerolla( kp()->asetus("LaskuKateistili").toInt());
    }




}

void LaskuDialogi::vieMalliin()
{
    model->asetaErapaiva( ui->eraDate->date());
    model->asetaLisatieto( ui->lisatietoEdit->toPlainText());
    model->asetaOsoite(ui->osoiteEdit->toPlainText());
    model->asetaToimituspaiva(ui->toimitusDate->date());
    model->asetaLaskunsaajannimi(ui->saajaEdit->text());
    model->asetaKirjausperuste(ui->perusteCombo->currentData().toInt());
}
