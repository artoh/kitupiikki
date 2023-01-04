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

#include "naytin/naytinikkuna.h"

#include "db/kirjanpito.h"
#include "ilmoitintuottaja.h"
#include "pilvi/pilvimodel.h"
#include "kieli/kielet.h"

#include "alvilmoitustenmodel.h"
#include "alvkaudet.h"

AlvSivu::AlvSivu() :
    ui(new Ui::AlvSivu),
    ilmoitin(new IlmoitinTuottaja(this))
{
    ui->setupUi(this);

    connect( kp()->alvIlmoitukset()->kaudet(), &AlvKaudet::haettu, this, &AlvSivu::paivitaKaudet);

    ui->ilmoituksetView->setModel( kp()->alvIlmoitukset() );
    ui->ilmoituksetView->setSelectionMode(QAbstractItemView::SingleSelection);
    ui->ilmoituksetView->setSelectionBehavior(QAbstractItemView::SelectRows);

    connect(ui->alkaaEdit, &QDateEdit::dateChanged, this, &AlvSivu::paivitaLoppu);
    connect(ui->paattyyEdit, &QDateEdit::dateChanged, this, &AlvSivu::paivitaErapaiva);
    connect( kp()->alvIlmoitukset(), &AlvIlmoitustenModel::modelReset, this, &AlvSivu::siirrySivulle);

    connect( ui->tilitaNappi, SIGNAL(clicked(bool)), this, SLOT(ilmoita()));
    connect( ui->tilitysNappi, SIGNAL(clicked(bool)), this, SLOT(naytaIlmoitus()));
    connect(ui->ilmoituksetView->selectionModel(), SIGNAL(currentRowChanged(QModelIndex,QModelIndex)), this, SLOT(riviValittu()));
    connect( ui->poistaTilitysNappi, &QPushButton::clicked, this, &AlvSivu::poistaIlmoitus);
    connect( ui->ilmoitinNappi, &QPushButton::clicked, this, &AlvSivu::tallennaIlmoitinAineisto);

    connect( ui->kaudelleCombo, &QComboBox::currentTextChanged, this, &AlvSivu::kausiValittu);

}

AlvSivu::~AlvSivu()
{
    delete ui;
}

void AlvSivu::siirrySivulle()
{
    alustaa_ = true;
    riviValittu();      // Jotta napit harmaantuvat
    ui->alkaaEdit->setDate( kp()->alvIlmoitukset()->viimeinenIlmoitus().addDays(1) );

    for(int i=0; i<4; i++)
        ui->ilmoituksetView->horizontalHeader()->resizeSection(i, ui->ilmoituksetView->width() / 5 );

    ui->ilmoitinNappi->setVisible( kp()->pilvi() && !kp()->pilvi()->service("ilmoitinkoodi").isEmpty() );

    paivitaKaudet();

    alustaa_ = false;

}


void AlvSivu::paivitaLoppu()
{
    QDate alkupvm = ui->alkaaEdit->date();
    int kaudenPituus = kp()->asetukset()->luku(AsetusModel::AlvKausi);

    if( alkupvm.day() != 1)
        ui->alkaaEdit->setDate(QDate(alkupvm.year(), alkupvm.month(), 1));
    else
        ui->paattyyEdit->setDate( alkupvm.addMonths( kaudenPituus ).addDays(-1)  );
}

void AlvSivu::paivitaErapaiva()
{
    QDate loppupvm = ui->paattyyEdit->date();
    if( loppupvm.day() != loppupvm.daysInMonth()) {
        ui->paattyyEdit->setDate(QDate( loppupvm.year(), loppupvm.month(), loppupvm.daysInMonth()));
    } else {
        QDate erapaiva = kp()->alvIlmoitukset()->erapaiva(loppupvm);

        ui->erapaivaLabel->setText( erapaiva.toString("dd.MM.yyyy") );

        if( kp()->paivamaara().daysTo( erapaiva) < 1)
            ui->erapaivaLabel->setStyleSheet("color: red;");
        else if( kp()->paivamaara().daysTo( erapaiva) < 6)
            ui->erapaivaLabel->setStyleSheet("color: yellow;");
        else
            ui->erapaivaLabel->setStyleSheet("color: black;");
    }
    ui->tilitaNappi->setEnabled( loppupvm > kp()->tilitpaatetty() &&
                                 !kp()->alvIlmoitukset()->onkoIlmoitettu(loppupvm) &&
                                 !kp()->alvIlmoitukset()->onkoIlmoitettu(ui->alkaaEdit->date()));
}

void AlvSivu::ilmoita()
{
    AlvIlmoitusDialog *dlg = new AlvIlmoitusDialog(this);
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

    QDate pvm = index.data(AlvIlmoitustenModel::PaattyyRooli).toDate();
    AlvKausi kausi = kp()->alvIlmoitukset()->kaudet()->kausi(pvm);
    bool voiIlmoittaa = kausi.tila() == AlvKausi::PUUTTUVA || kausi.tila() == AlvKausi::EIKAUTTA;

    ui->ilmoitinNappi->setEnabled( index.isValid() &&
                                   voiIlmoittaa &&
                                   ilmoitin->voikoMuodostaa(index.data(AlvIlmoitustenModel::MapRooli).toMap()));

}

void AlvSivu::tallennaIlmoitinAineisto()
{
    int tositeId = kp()->alvIlmoitukset()->data( ui->ilmoituksetView->selectionModel()->currentIndex() , AlvIlmoitustenModel::TositeIdRooli ).toInt();
    if(tositeId)
        ilmoitin->tallennaAineisto(tositeId);
}


void AlvSivu::paivitaKaudet()
{
    ui->kaudelleCombo->clear();
    QList<AlvKausi> kaudet = kp()->alvIlmoitukset()->kaudet()->kaudet();
    if(kaudet.count()) {
        for(const auto& kausi : qAsConst(kaudet)) {
            if( !kp()->alvIlmoitukset()->onkoIlmoitettu( kausi.loppupvm() ) &&
                kp()->tilitpaatetty() < kausi.loppupvm() &&
                kausi.tila() != AlvKausi::ERAANTYNYT  ){
                ui->kaudelleCombo->addItem(
                            kausi.tila() == AlvKausi::PUUTTUVA ? QIcon(":/pic/uusitiedosto.png") : QIcon(":/pic/muokkaa.png"),
                            QString("%1 - %2").arg( kausi.alkupvm().toString("dd.MM.yyyy"), kausi.loppupvm().toString("dd.MM.yyyy") ),
                            kausi.loppupvm() );
            }
        }
        if( ui->kaudelleCombo->count()) {
            ui->kaudelleCombo->setCurrentIndex( ui->kaudelleCombo->count() - 1 );

            ui->ilmoitusGroup->setVisible(true);

            ui->kaudelleCombo->setVisible(true);
            ui->kaudelleLabel->setVisible(true);

            ui->alkaaEdit->setVisible(false);
            ui->paattyyEdit->setVisible(false);
            ui->alkaaLabel->setVisible(false);
            ui->paattyyLabel->setVisible(false);

        } else {
            ui->ilmoitusGroup->setVisible(false);
        }
    } else {
        ui->ilmoitusGroup->setVisible(true);

        ui->kaudelleCombo->setVisible(false);
        ui->kaudelleLabel->setVisible(false);

        ui->alkaaEdit->setVisible(true);
        ui->paattyyEdit->setVisible(true);
        ui->alkaaLabel->setVisible(true);
        ui->paattyyLabel->setVisible(true);
    }

}

void AlvSivu::kausiValittu()
{
    QDate pvm = ui->kaudelleCombo->currentData().toDate();
    if( pvm.isValid()) {
        AlvKausi kausi = kp()->alvIlmoitukset()->kaudet()->kausi(pvm);
        if( kausi.tila() != AlvKausi::EIKAUTTA) {
            ui->alkaaEdit->setDate(kausi.alkupvm());
            ui->paattyyEdit->setDate(kausi.loppupvm());
            ui->erapaivaLabel->setText( kausi.erapvm().toString("dd.MM.yyyy"));
            if( kp()->paivamaara().daysTo( kausi.erapvm()) < 1)
                ui->erapaivaLabel->setStyleSheet("color: red;");
            else if( kp()->paivamaara().daysTo( kausi.erapvm()) < 6)
                ui->erapaivaLabel->setStyleSheet("color: yellow;");
            else
                ui->erapaivaLabel->setStyleSheet("color: black;");
            ui->tilitaNappi->setEnabled(true);
        }
    }
}


