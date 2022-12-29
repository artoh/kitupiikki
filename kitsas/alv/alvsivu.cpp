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

#include "alvilmoitusdialog.h"
#include "alvlaskelma.h"

#include "ui_maksuperusteinen.h"

#include "naytin/naytinikkuna.h"

#include "db/kirjanpito.h"
#include "db/yhteysmodel.h"
#include "ilmoitintuottaja.h"
#include "pilvi/pilvimodel.h"
#include "kieli/kielet.h"

#include "alvilmoitustenmodel.h"

AlvSivu::AlvSivu() :
    ui(new Ui::AlvSivu),
    ilmoitin(new IlmoitinTuottaja(this))
{
    ui->setupUi(this);

    ui->kausiCombo->addItem(tr("Kuukausi"),1);
    ui->kausiCombo->addItem(tr("Neljännesvuosi"),3);
    ui->kausiCombo->addItem(tr("Vuosi"), 12);
    ui->ilmoituksetView->setModel( kp()->alvIlmoitukset() );
    ui->ilmoituksetView->setSelectionMode(QAbstractItemView::SingleSelection);
    ui->ilmoituksetView->setSelectionBehavior(QAbstractItemView::SelectRows);
    paivitaMaksuAlvTieto();

    connect(ui->kausiCombo, &QComboBox::currentTextChanged, this, [this] { if(!alustaa_) kp()->asetukset()->aseta("AlvKausi", ui->kausiCombo->currentData().toInt()); this->paivitaLoppu();});
    connect(ui->alkaaEdit, &QDateEdit::dateChanged, this, &AlvSivu::paivitaLoppu);
    connect(ui->paattyyEdit, &QDateEdit::dateChanged, this, &AlvSivu::paivitaErapaiva);
    connect( kp()->alvIlmoitukset(), &AlvIlmoitustenModel::modelReset, this, &AlvSivu::siirrySivulle);

    connect( ui->tilitaNappi, SIGNAL(clicked(bool)), this, SLOT(ilmoita()));
    connect( ui->tilitysNappi, SIGNAL(clicked(bool)), this, SLOT(naytaIlmoitus()));
    connect(ui->ilmoituksetView->selectionModel(), SIGNAL(currentRowChanged(QModelIndex,QModelIndex)), this, SLOT(riviValittu()));
    connect( ui->maksuperusteNappi, SIGNAL(clicked(bool)), this, SLOT(maksuAlv()));
    connect( ui->poistaTilitysNappi, &QPushButton::clicked, this, &AlvSivu::poistaIlmoitus);
    connect( ui->ilmoitinNappi, &QPushButton::clicked, this, &AlvSivu::tallennaIlmoitinAineisto);

}

AlvSivu::~AlvSivu()
{
    delete ui;
}

void AlvSivu::siirrySivulle()
{
    alustaa_ = true;
    ui->kausiCombo->setCurrentIndex( ui->kausiCombo->findData( kp()->asetukset()->asetus(AsetusModel::AlvKausi) ) );
    riviValittu();      // Jotta napit harmaantuvat
    ui->alkaaEdit->setDate( kp()->alvIlmoitukset()->viimeinenIlmoitus().addDays(1) );
    paivitaMaksuAlvTieto();

    for(int i=0; i<3; i++)
        ui->ilmoituksetView->horizontalHeader()->resizeSection(i, ui->ilmoituksetView->width() / 4 );

    ui->kausiCombo->setEnabled( kp()->yhteysModel()->onkoOikeutta(YhteysModel::ASETUKSET) );
    ui->maksuperusteNappi->setEnabled( kp()->yhteysModel()->onkoOikeutta(YhteysModel::ASETUKSET));

    ui->ilmoitinNappi->setVisible( kp()->pilvi() && !kp()->pilvi()->service("ilmoitinkoodi").isEmpty() );

    alustaa_ = false;

}


void AlvSivu::paivitaLoppu()
{
    QDate alkupvm = ui->alkaaEdit->date();
    if( alkupvm.day() != 1)
        ui->alkaaEdit->setDate(QDate(alkupvm.year(), alkupvm.month(), 1));
    else
        ui->paattyyEdit->setDate( alkupvm.addMonths( ui->kausiCombo->currentData().toInt() ).addDays(-1)  );
}

void AlvSivu::paivitaErapaiva()
{
    QDate loppupvm = ui->paattyyEdit->date();
    if( loppupvm.day() != loppupvm.daysInMonth()) {
        ui->paattyyEdit->setDate(QDate( loppupvm.year(), loppupvm.month(), loppupvm.daysInMonth()));
    } else {
        QDate erapaiva = AlvIlmoitustenModel::erapaiva(loppupvm);

        ui->erapaivaLabel->setText( erapaiva.toString("dd.MM.yyyy") );

        if( kp()->paivamaara().daysTo( erapaiva) < 3)
            ui->erapaivaLabel->setStyleSheet("color: red;");
        else
            ui->erapaivaLabel->setStyleSheet("color: black;");
    }
    ui->tilitaNappi->setEnabled( loppupvm > kp()->tilitpaatetty() &&
                                 !kp()->alvIlmoitukset()->onkoIlmoitettu(loppupvm) &&
                                 !kp()->alvIlmoitukset()->onkoIlmoitettu(ui->alkaaEdit->date()));
}

void AlvSivu::ilmoita()
{
    AlvIlmoitusDialog *dlg = new AlvIlmoitusDialog();
    AlvLaskelma *laskelma = new AlvLaskelma(dlg, Kielet::instanssi()->nykyinen());

    connect(laskelma, &AlvLaskelma::valmis, dlg, &AlvIlmoitusDialog::naytaLaskelma);
    connect(laskelma, &AlvLaskelma::tallennettu, this, &AlvSivu::siirrySivulle);
    laskelma->laske(ui->alkaaEdit->date(), ui->paattyyEdit->date());

}

void AlvSivu::naytaIlmoitus()
{
    // Ilmoitus on tositteen ensimmäinen liite
    int tositeId = kp()->alvIlmoitukset()->data( ui->ilmoituksetView->selectionModel()->currentIndex() , AlvIlmoitustenModel::TositeIdRooli ).toInt();

    NaytinIkkuna::naytaLiite(tositeId, "alv");

}


void AlvSivu::poistaIlmoitus()
{
    int tositeId = kp()->alvIlmoitukset()->data( ui->ilmoituksetView->selectionModel()->currentIndex() , AlvIlmoitustenModel::TositeIdRooli ).toInt();

    if( QMessageBox::question(this, tr("Alv-ilmoituksen poistaminen"), tr("Haluatko todellakin poistaa valitun alv-ilmoituksen?\n"
                                                                          "Poistamisen jälkeen sinun on laadittava uusi alv-ilmoitus."),
                              QMessageBox::Yes | QMessageBox::Cancel) == QMessageBox::Yes   )
    {        
        KpKysely *kysely = kpk(QString("/tositteet/%1").arg(tositeId), KpKysely::DELETE);
        connect( kysely, &KpKysely::vastaus, kp()->alvIlmoitukset(), &AlvIlmoitustenModel::lataa);
        kysely->kysy();

    }
}

void AlvSivu::riviValittu()
{
    QModelIndex index = ui->ilmoituksetView->selectionModel()->currentIndex();

    ui->tilitysNappi->setEnabled( index.isValid() );
    ui->poistaTilitysNappi->setEnabled( index.isValid() &&                                        
                                        index.data(AlvIlmoitustenModel::PaattyyRooli).toDate() > kp()->tilitpaatetty() );
    ui->ilmoitinNappi->setEnabled( index.isValid() &&
                                   ilmoitin->voikoMuodostaa(index.data(AlvIlmoitustenModel::MapRooli).toMap()));

}

void AlvSivu::tallennaIlmoitinAineisto()
{
    int tositeId = kp()->alvIlmoitukset()->data( ui->ilmoituksetView->selectionModel()->currentIndex() , AlvIlmoitustenModel::TositeIdRooli ).toInt();
    if(tositeId)
        ilmoitin->tallennaAineisto(tositeId);
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

    connect( ui.ohjeNappi, &QPushButton::clicked, [] { kp()->ohje("alv/maksuperusteinen");} );

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
        ui->maksuAlv->setText( tr("%1 - %2").arg(alkaa.toString("dd.MM.yyyy"), loppuu.toString("dd.MM.yyyy")));
}


