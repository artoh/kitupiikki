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

#include <QSortFilterProxyModel>
#include <QScrollBar>

#include <QMessageBox>

#include "tilinavaus.h"
#include "tilinavausmodel.h"
#include "avauseradlg.h"

Tilinavaus::Tilinavaus(QWidget *parent) : MaaritysWidget(parent)
{
    ui = new Ui::Tilinavaus;
    ui->setupUi(this);

    ui->siirryEdit->setValidator(new QIntValidator(this));


    connect( ui->henkilostoSpin, SIGNAL(valueChanged(int)), this, SLOT(hlostoMuutos()));

    connect( ui->kaikkiNappi, &QPushButton::clicked, this, &Tilinavaus::naytaPiilotetut);
    connect( ui->kirjauksetNappi, &QPushButton::clicked, this, &Tilinavaus::naytaVainKirjaukset);

    connect( ui->etsiEdit, &QLineEdit::textEdited, ui->tiliView, &TilinAvausView::suodata);
    connect( ui->tiliView, &QTableView::activated, this, &Tilinavaus::erittely);
    connect( ui->tiliView, &QTableView::clicked, this, &Tilinavaus::erittely);
    connect( ui->siirryEdit, &QLineEdit::textEdited, this, &Tilinavaus::siirry);

    connect( ui->tiliView->avausModel() , &TilinavausModel::tilasto, this, &Tilinavaus::info);
    connect( ui->kkCheck, &QCheckBox::clicked, ui->tiliView->avausModel(), &TilinavausModel::setKuukausittain);
}

Tilinavaus::~Tilinavaus()
{
    delete ui;
}


void Tilinavaus::hlostoMuutos()
{
    // Tositetta voi käyttää vain, jos ei tallentamatonta!
    emit tallennaKaytossa( onkoMuokattu());
}


void Tilinavaus::naytaPiilotetut(bool naytetaanko)
{
    ui->tiliView->naytaPiilotetut(naytetaanko);
    if(naytetaanko) ui->kirjauksetNappi->setChecked(false);
}

void Tilinavaus::naytaVainKirjaukset(bool naytetaanko)
{
    ui->tiliView->naytaVainKirjaukset(naytetaanko);
    if(naytetaanko) ui->kaikkiNappi->setChecked(false);
}

void Tilinavaus::erittely(const QModelIndex &index)
{
    if( index.data(TilinavausModel::ErittelyRooli).toInt() != TilinavausModel::EI_ERITTELYA && index.column() == TilinavausModel::ERITTELY) {
        int tili = index.data(TilinavausModel::NumeroRooli).toInt();
        AvausEraDlg dlg(ui->tiliView->avausModel(), tili, this);
        dlg.exec();
    }
}

void Tilinavaus::info(qlonglong vastaavaa, qlonglong vastattavaa, qlonglong tulos)
{
    emit tallennaKaytossa(onkoMuokattu());

    ui->infoLabel->setText(QString("Vastaavaa \t%L1 € \t\tTulos\t %L2 € \nVastattavaa \t%L3 €")
                           .arg( ( vastaavaa / 100.0 ), 10,'f',2)
                           .arg( ( tulos / 100.0 ), 10,'f',2)
                           .arg( ( vastattavaa / 100.0 ), 10,'f',2) );

    ui->poikkeusLabel->clear();
    if( vastaavaa != vastattavaa)
        ui->poikkeusLabel->setText(QString("%L1 €").arg( ( qAbs(vastaavaa-vastattavaa) / 100.0 ), 10,'f',2));
}

void Tilinavaus::siirry(const QString &minne)
{
    if( !minne.isEmpty()) {
        for(int i=0; i < ui->tiliView->model()->rowCount(); i++) {
            QModelIndex index = ui->tiliView->model()->index(i,0);

            if( QString::number(index.data(TiliModel::NroRooli).toInt()).startsWith(minne) ||
                index.data(TiliModel::NimiRooli).toString().startsWith(minne))
            {
                // Valitsee tämän rivin
                ui->tiliView->setCurrentIndex(index);
                // Ja scrollaa rivin näkyviin
                ui->tiliView->verticalScrollBar()->setSliderPosition(i);
                return;
            }
        }
    }
}

bool Tilinavaus::nollaa()
{

    ui->tiliView->nollaa();
    ui->henkilostoSpin->setValue(kp()->tilikaudet()->tilikausiIndeksilla(0).henkilosto());
    ui->kkCheck->setChecked( ui->tiliView->avausModel()->kuukausittain() );
    emit tallennaKaytossa(onkoMuokattu());
    return true;
}

bool Tilinavaus::tallenna()
{            
    // #40 Model tallennetaan vain, jos sitä on muokattu
    if( ui->tiliView->avausModel()->onkoMuokattu()) {

        if( !ui->poikkeusLabel->text().isEmpty() ) {
            if( QMessageBox::question(this, tr("Tilinavaus ei täsmää"),
                                      tr("Tilinavauksen Vastaavaa- ja Vastattavaa-määrät eivät täsmää.\n\n"
                                         "Tilinavaus voidaan siksi tallentaa vain luonnoksena, eikä tilinavaus näy "
                                         "kirjanpidon tilien saldoissa ennen kuin se on korjattu.\n\n"
                                         "Haluatko tallentaa tilinavauksen luonnoksena?"),
                                      QMessageBox::Yes | QMessageBox::Cancel, QMessageBox::Cancel) == QMessageBox::Yes) {
                ui->tiliView->avausModel()->tallenna(Tosite::LUONNOS);
            } else {
                return false;
            }
        } else {
            ui->tiliView->avausModel()->tallenna(Tosite::KIRJANPIDOSSA);
        }
    }

    Tilikausi kausi = kp()->tilikaudet()->tilikausiPaivalle(kp()->asetukset()->pvm("TilinavausPvm"));
    kausi.set("henkilosto", ui->henkilostoSpin->value());
    kausi.tallenna();


    emit tallennaKaytossa(onkoMuokattu());

    return true;
}

bool Tilinavaus::onkoMuokattu()
{
    return ui->tiliView->avausModel()->onkoMuokattu() ||
          ui->henkilostoSpin->value() != kp()->tilikaudet()->viiteIndeksilla(0).henkilosto();
}
