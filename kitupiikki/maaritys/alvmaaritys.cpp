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
#include <QMessageBox>

#include "alvmaaritys.h"
#include "ui_arvonlisavero.h"
#include "db/kirjanpito.h"
#include "db/tositemodel.h"

#include "alvilmoitusdialog.h"
#include "ui_maksuperusteinen.h"

#include "naytin/naytinikkuna.h"

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
    paivitaMaksuAlvTieto();


    connect( ui->viimeisinEdit, SIGNAL(dateChanged(QDate)), this, SLOT(paivitaSeuraavat()));
    connect(ui->kausiCombo, SIGNAL(currentIndexChanged(int)), this, SLOT(paivitaSeuraavat()));
    connect( ui->tilitaNappi, SIGNAL(clicked(bool)), this, SLOT(ilmoita()));
    connect( ui->tilitysNappi, SIGNAL(clicked(bool)), this, SLOT(naytaIlmoitus()));
    connect( ui->erittelyNappi, SIGNAL(clicked(bool)), this, SLOT(naytaErittely()));
    connect(ui->ilmoituksetView->selectionModel(), SIGNAL(currentRowChanged(QModelIndex,QModelIndex)), this, SLOT(riviValittu()));
    connect( ui->maksuperusteNappi, SIGNAL(clicked(bool)), this, SLOT(maksuAlv()));
    connect( ui->poistaTilitysNappi, &QPushButton::clicked, this, &AlvMaaritys::poistaIlmoitus);

}

bool AlvMaaritys::nollaa()
{
    ui->kausiCombo->setCurrentIndex( ui->kausiCombo->findData( kp()->asetukset()->luku("AlvKausi") ) );
    ui->viimeisinEdit->setDate( kp()->asetukset()->pvm("AlvIlmoitus"));
    riviValittu();      // Jotta napit harmaantuvat
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

    ui->seuraavaLabel->setText( QString("%1 - %2").arg( seuraavaAlkaa.toString("dd.MM.yyyy"))
                                                        .arg(seuraavaLoppuu.toString("dd.MM.yyyy")) );
    ui->erapaivaLabel->setText( erapaiva(seuraavaLoppuu).toString("dd.MM.yyyy") );

    if( kp()->paivamaara().daysTo( erapaiva(seuraavaLoppuu) ) < 3)
        ui->erapaivaLabel->setStyleSheet("color: red;");
    else
        ui->erapaivaLabel->setStyleSheet("color: black;");

    ui->tilitaNappi->setEnabled( seuraavaLoppuu < kp()->tilikaudet()->kirjanpitoLoppuu() );

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

    NaytinIkkuna::naytaLiite(tositeId, 1);

}

void AlvMaaritys::naytaErittely()
{
    // Erittely on tositteen toinen liite
    int tositeId = model->data( ui->ilmoituksetView->selectionModel()->currentIndex() , AlvIlmoitustenModel::TositeIdRooli ).toInt();

    NaytinIkkuna::naytaLiite(tositeId, 2);
}

void AlvMaaritys::poistaIlmoitus()
{
    int tositeId = model->data( ui->ilmoituksetView->selectionModel()->currentIndex() , AlvIlmoitustenModel::TositeIdRooli ).toInt();

    TositeModel tosite(kp()->tietokanta());

    if( QMessageBox::question(this, tr("Alv-ilmoituksen poistaminen"), tr("Haluatko todellakin poistaa viimeisimmän alv-ilmoituksen?\n"
                                                                          "Poistamisen jälkeen sinun on laadittava uusi alv-ilmoitus."),
                              QMessageBox::Yes | QMessageBox::Cancel) == QMessageBox::Yes   )
    {
        tosite.lataa( tositeId );
        tosite.poista();
        model->lataa();
        nollaa();
    }
}

void AlvMaaritys::riviValittu()
{
    QModelIndex index = ui->ilmoituksetView->selectionModel()->currentIndex();

    ui->tilitysNappi->setEnabled( index.isValid() );
    ui->erittelyNappi->setEnabled( index.isValid() );
    ui->poistaTilitysNappi->setEnabled( index.isValid() &&
                                        index.data(AlvIlmoitustenModel::PaattyyRooli).toDate() == kp()->asetukset()->pvm("AlvIlmoitus")  &&
                                        index.data(AlvIlmoitustenModel::EraPvmRooli).toDate() >= kp()->paivamaara() );

}

void AlvMaaritys::maksuAlv()
{
    if( !kp()->tilit()->tiliTyypilla(TiliLaji::KOHDENTAMATONALVSAATAVA).onkoValidi() || !kp()->tilit()->tiliTyypilla(TiliLaji::KOHDENTAMATONALVVELKA).onkoValidi() )
    {
        QMessageBox::critical(nullptr, tr("Tilikartta puutteellinen"), tr("Maksuperusteiseen arvonlisäveroon tarvittavat kohdentamattoman arvonlisäverovelan ja/tai "
                                                                    "arvonlisäverosaatavien tilit puuttuvat.\n"
                                                                    "Ottaaksesi maksuperusteisen arvonlisäveron käyttöön lisää tarvittavat tilit "
                                                                    "tilikarttaasi"));
        return;
    }


    QDialog dlg;
    Ui::Maksuperusteinen ui;
    ui.setupUi(&dlg);

    QDate alkaa = kp()->asetukset()->pvm("MaksuAlvAlkaa");
    QDate loppuu = kp()->asetukset()->pvm("MaksuAlvLoppuu");

    // Oletuksena uudelle aloitukselle on seuraavan kuukauden alku
    if( !alkaa.isValid())
    {
        alkaa = kp()->paivamaara();
        alkaa = alkaa.addMonths(1);
        alkaa.setDate( alkaa.year(), alkaa.month(), 1 );
    }
    if( !loppuu.isValid())
    {
        loppuu.setDate( kp()->tilikaudet()->kirjanpitoLoppuu().year() + 1, 1, 1 );
    }

    ui.alkaaDate->setMinimumDate( kp()->tilikaudet()->kirjanpitoAlkaa());
    ui.paattyyDate->setMinimumDate( kp()->tilikaudet()->kirjanpitoAlkaa());

    ui.alkaaCheck->setChecked( kp()->asetukset()->onko("MaksuAlvAlkaa") );
    ui.alkaaDate->setEnabled( kp()->asetukset()->onko("MaksuAlvAlkaa") );
    ui.alkaaDate->setDate( alkaa );
    ui.paattyyCheck->setChecked( kp()->asetukset()->onko("MaksuAlvLoppuu"));
    ui.paattyyDate->setEnabled( kp()->asetukset()->onko("MaksuAlvLoppuu"));
    ui.paattyyDate->setDate( loppuu );

    connect( ui.ohjeNappi, &QPushButton::clicked, [] { kp()->ohje("alv");} );

    if( dlg.exec() == QDialog::Accepted)
    {
        if( ui.alkaaCheck->isChecked())
            kp()->asetukset()->aseta("MaksuAlvAlkaa", ui.alkaaDate->date());
        else
            kp()->asetukset()->poista("MaksuAlvAlkaa");

        if( ui.paattyyCheck->isChecked())
            kp()->asetukset()->aseta("MaksuAlvLoppuu", ui.paattyyDate->date());
        else
            kp()->asetukset()->poista("MaksuAlvLoppuu");
        paivitaMaksuAlvTieto();
    }
}

void AlvMaaritys::paivitaMaksuAlvTieto()
{
    QDate alkaa = kp()->asetukset()->pvm("MaksuAlvAlkaa");
    QDate loppuu = kp()->asetukset()->pvm("MaksuAlvLoppuu");

    if( !alkaa.isValid())
        ui->maksuAlv->setText(tr("Ei käytössä"));
    else if( !loppuu.isValid())
        ui->maksuAlv->setText(tr("Käytössä %1 alkaen").arg(alkaa.toString("dd.MM.yyyy")));
    else
        ui->maksuAlv->setText( tr("%1 - %2").arg(alkaa.toString("dd.MM.yyyy")).arg(loppuu.toString("dd.MM.yyyy")));
}

QDate AlvMaaritys::erapaiva(const QDate &loppupaiva)
{
    return loppupaiva.addDays(1).addMonths(1).addDays(11);
}

