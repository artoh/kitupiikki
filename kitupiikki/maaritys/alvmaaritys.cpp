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

#include <QDesktopServices>
#include <QUrl>

#include "alvmaaritys.h"
#include "ui_arvonlisavero.h"
#include "db/kirjanpito.h"
#include "db/tositemodel.h"

#include "alvilmoitusdialog.h"



AlvMaaritys::AlvMaaritys() :
    ui(new Ui::AlvMaaritys)
{
    ui->setupUi(this);
    model = new AlvIlmoitustenModel(this);

    ui->kausiCombo->addItem("Kuukausi",QVariant(1));
    ui->kausiCombo->addItem("Neljännesvuosi",QVariant(3));
    ui->kausiCombo->addItem("Vuosi", QVariant(12));
    ui->ilmoituksetView->setModel( model );
    ui->ilmoituksetView->setSelectionMode(QAbstractItemView::SingleSelection);
    ui->ilmoituksetView->setSelectionBehavior(QAbstractItemView::SelectRows);


    connect( ui->viimeisinEdit, SIGNAL(dateChanged(QDate)), this, SLOT(paivitaSeuraavat()));
    connect(ui->kausiCombo, SIGNAL(currentIndexChanged(int)), this, SLOT(paivitaSeuraavat()));
    connect( ui->tilitaNappi, SIGNAL(clicked(bool)), this, SLOT(ilmoita()));
    connect( ui->tilitysNappi, SIGNAL(clicked(bool)), this, SLOT(naytaIlmoitus()));
    connect( ui->erittelyNappi, SIGNAL(clicked(bool)), this, SLOT(naytaErittely()));
    connect(ui->ilmoituksetView->selectionModel(), SIGNAL(currentRowChanged(QModelIndex,QModelIndex)), this, SLOT(riviValittu()));

}

bool AlvMaaritys::nollaa()
{
    ui->kausiCombo->setCurrentIndex( ui->kausiCombo->findData( kp()->asetukset()->luku("AlvKausi") ) );
    ui->viimeisinEdit->setDate( kp()->asetukset()->pvm("AlvIlmoitus"));
    return true;
}

bool AlvMaaritys::onkoMuokattu()
{
    return ui->kausiCombo->currentData().toInt() != kp()->asetukset()->luku("AlvKausi") ||
            ui->viimeisinEdit->date() != kp()->asetukset()->pvm("AlvIlmoitus");
}

bool AlvMaaritys::tallenna()
{
    kp()->asetukset()->aseta("AlvKausi", ui->kausiCombo->currentData().toInt());
    kp()->asetukset()->aseta("AlvIlmoitus", ui->viimeisinEdit->date());
    return true;
}

void AlvMaaritys::paivitaSeuraavat()
{
    QDate viimeisin = ui->viimeisinEdit->date();
    int kausikk = ui->kausiCombo->currentData().toInt();

    seuraavaAlkaa = viimeisin.addDays(1);
    seuraavaLoppuu = seuraavaAlkaa.addMonths( kausikk ).addDays(-1);

    ui->seuraavaLabel->setText( QString("%1 - %2").arg( seuraavaAlkaa.toString(Qt::SystemLocaleShortDate))
                                                        .arg(seuraavaLoppuu.toString(Qt::SystemLocaleShortDate)) );
    ui->erapaivaLabel->setText( erapaiva(seuraavaLoppuu).toString(Qt::SystemLocaleShortDate) );

    if( kp()->paivamaara().daysTo( erapaiva(seuraavaLoppuu) ) < 3)
        ui->erapaivaLabel->setStyleSheet("color: red;");
    else
        ui->erapaivaLabel->setStyleSheet("color: black;");

    emit tallennaKaytossa(onkoMuokattu());
}

void AlvMaaritys::ilmoita()
{
    QDate ilmoitettu = AlvIlmoitusDialog::teeAlvIlmoitus(seuraavaAlkaa, seuraavaLoppuu);
    if( ilmoitettu.isValid())
    {
        kp()->asetukset()->aseta("AlvIlmoitus", ilmoitettu);
        ui->viimeisinEdit->setDate(ilmoitettu);
        model->lataa();
        ui->ilmoituksetView->resizeColumnsToContents();
    }
}

void AlvMaaritys::naytaIlmoitus()
{
    // Ilmoitus on tositteen ensimmäinen liite
    int tositeId = model->data( ui->ilmoituksetView->selectionModel()->currentIndex() , AlvIlmoitustenModel::TositeIdRooli ).toInt();

    TositeModel tosite( kp()->tietokanta());
    tosite.lataa(tositeId);
    QString polku = tosite.liiteModel()->liitePolku(1);
    QDesktopServices::openUrl( QUrl( polku ) );


}

void AlvMaaritys::naytaErittely()
{
    // Erittely on tositteen toinen liite
    int tositeId = model->data( ui->ilmoituksetView->selectionModel()->currentIndex() , AlvIlmoitustenModel::TositeIdRooli ).toInt();

    TositeModel tosite( kp()->tietokanta());
    tosite.lataa(tositeId);
    QString polku = tosite.liiteModel()->liitePolku(2);
    QDesktopServices::openUrl( QUrl( polku ) );
}

void AlvMaaritys::riviValittu()
{
    ui->tilitysNappi->setEnabled( ui->ilmoituksetView->selectionModel()->currentIndex().isValid() );
    ui->erittelyNappi->setEnabled( ui->ilmoituksetView->selectionModel()->currentIndex().isValid() );
}

QDate AlvMaaritys::erapaiva(const QDate &loppupaiva)
{
    return loppupaiva.addMonths(1).addDays(12);
}

