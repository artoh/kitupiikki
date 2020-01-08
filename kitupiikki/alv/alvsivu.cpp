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

#include "alvsivu.h"
#include "ui_arvonlisavero.h"
#include "db/kirjanpito.h"
#include "db/tositemodel.h"

#include "alvilmoitusdialog.h"
#include "ui_maksuperusteinen.h"

#include "naytin/naytinikkuna.h"

AlvSivu::AlvSivu() :
    ui(new Ui::AlvSivu)
{
    ui->setupUi(this);
    model = new AlvIlmoitustenModel(this);

    ui->kausiCombo->addItem("Kuukausi",QVariant(1));
    ui->kausiCombo->addItem("Neljännesvuosi",QVariant(3));
    ui->kausiCombo->addItem("Vuosi", QVariant(12));
    ui->ilmoituksetView->setModel( model );
    ui->ilmoituksetView->setSelectionMode(QAbstractItemView::SingleSelection);
    ui->ilmoituksetView->setSelectionBehavior(QAbstractItemView::SelectRows);

    connect( ui->viimeisinEdit, &QDateEdit::editingFinished, this, &AlvSivu::paivitaSeuraavat);
    connect(ui->kausiCombo, SIGNAL(currentIndexChanged(int)), this, SLOT(paivitaSeuraavat()));
    connect( ui->tilitaNappi, SIGNAL(clicked(bool)), this, SLOT(ilmoita()));
    connect( ui->tilitysNappi, SIGNAL(clicked(bool)), this, SLOT(naytaIlmoitus()));
    connect( ui->erittelyNappi, SIGNAL(clicked(bool)), this, SLOT(naytaErittely()));
    connect(ui->ilmoituksetView->selectionModel(), SIGNAL(currentRowChanged(QModelIndex,QModelIndex)), this, SLOT(riviValittu()));
    connect( ui->maksuperusteNappi, SIGNAL(clicked(bool)), this, SLOT(maksuAlv()));
    connect( ui->poistaTilitysNappi, &QPushButton::clicked, this, &AlvSivu::poistaIlmoitus);

}

void AlvSivu::siirrySivulle()
{
    alustaa_ = true;
    ui->kausiCombo->setCurrentIndex( ui->kausiCombo->findData( kp()->asetukset()->luku("AlvKausi") ) );
    ui->viimeisinEdit->setDate( kp()->asetukset()->pvm("AlvIlmoitus"));

    paivitaSeuraavat();
    model->lataa();
    riviValittu();      // Jotta napit harmaantuvat
    paivitaMaksuAlvTieto();

    for(int i=0; i<3; i++)
        ui->ilmoituksetView->horizontalHeader()->resizeSection(i, ui->ilmoituksetView->width() / 4 );
    alustaa_ = false;
}


void AlvSivu::paivitaSeuraavat()
{
    QDate viimeisin = ui->viimeisinEdit->date();

    if( viimeisin.day() != viimeisin.daysInMonth())
    {
        viimeisin.setDate( viimeisin.year(), viimeisin.month(), viimeisin.daysInMonth() );
        ui->viimeisinEdit->setDate(viimeisin);
    }

    int kausikk = ui->kausiCombo->currentData().toInt();

    seuraavaAlkaa = viimeisin.addDays(1);
    seuraavaLoppuu = seuraavaAlkaa.addMonths( kausikk ).addDays(-1);

    ui->seuraavaLabel->setText( QString("%1 - %2").arg( seuraavaAlkaa.toString("dd.MM.yyyy"))
                                                        .arg(seuraavaLoppuu.toString("dd.MM.yyyy")) );

    ui->tilitaNappi->setEnabled( seuraavaLoppuu <= kp()->tilitpaatetty() );

    if( ui->viimeisinEdit->date() >= kp()->tilikaudet()->kirjanpitoAlkaa().addYears(-1) && !alustaa_ )
    {
        kp()->asetukset()->aseta("AlvKausi", ui->kausiCombo->currentData().toInt());
        kp()->asetukset()->aseta("AlvIlmoitus", ui->viimeisinEdit->date());
    }

    ui->erapaivaLabel->setText( erapaiva(seuraavaLoppuu).toString("dd.MM.yyyy") );

    if( kp()->paivamaara().daysTo( erapaiva(seuraavaLoppuu) ) < 3)
        ui->erapaivaLabel->setStyleSheet("color: red;");
    else
        ui->erapaivaLabel->setStyleSheet("color: black;");

}

void AlvSivu::ilmoita()
{

    QDate ilmoitettu = AlvIlmoitusDialog::teeAlvIlmoitus(seuraavaAlkaa, seuraavaLoppuu);
    if( ilmoitettu.isValid())
    {
        kp()->asetukset()->aseta("AlvIlmoitus", ilmoitettu);
        siirrySivulle();
    }
}

void AlvSivu::naytaIlmoitus()
{
    // Ilmoitus on tositteen ensimmäinen liite
    int tositeId = model->data( ui->ilmoituksetView->selectionModel()->currentIndex() , AlvIlmoitustenModel::TositeIdRooli ).toInt();

    NaytinIkkuna::naytaLiite(tositeId, 1);

}

void AlvSivu::naytaErittely()
{
    // Erittely on tositteen toinen liite
    int tositeId = model->data( ui->ilmoituksetView->selectionModel()->currentIndex() , AlvIlmoitustenModel::TositeIdRooli ).toInt();

    NaytinIkkuna::naytaLiite(tositeId, 2);
}

void AlvSivu::poistaIlmoitus()
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
        siirrySivulle();
    }
}

void AlvSivu::riviValittu()
{
    QModelIndex index = ui->ilmoituksetView->selectionModel()->currentIndex();

    ui->tilitysNappi->setEnabled( index.isValid() );
    ui->erittelyNappi->setEnabled( index.isValid() );
    ui->poistaTilitysNappi->setEnabled( index.isValid() &&
                                        index.data(AlvIlmoitustenModel::PaattyyRooli).toDate() > kp()->tilitpaatetty() );

}

void AlvSivu::maksuAlv()
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

void AlvSivu::paivitaMaksuAlvTieto()
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

QDate AlvSivu::erapaiva(const QDate &loppupaiva)
{
    if( kp()->asetukset()->luku("AlvKausi") == 12 )
        return loppupaiva.addMonths(2);

    return loppupaiva.addDays(1).addMonths(1).addDays(11);
}

