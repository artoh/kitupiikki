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

#include <QPushButton>
#include <QDebug>

#include "kirjausapuridialog.h"
#include "ui_kirjausapuridialog.h"

KirjausApuriDialog::KirjausApuriDialog(TositeModel *tositeModel, QWidget *parent) :
    QDialog(parent),
    ui(new Ui::KirjausApuriDialog),
    model(tositeModel)
{
    ui->setupUi(this);

    ui->nettoSpin->setVisible(false);
    ui->nettoLabel->setVisible(false);
    ui->alvCombo->setVisible(false);
    ui->alvlajiLabel->setVisible(false);
    ui->alvSpin->setVisible(false);
    ui->alvprossaLabel->setVisible(false);
    ui->kohdennusLabel->setVisible(false);
    ui->kohdennusCombo->setVisible(false);

    ui->kohdennusCombo->setModel( kp()->kohdennukset() );
    ui->kohdennusCombo->setModelColumn( KohdennusModel::NIMI );

    ui->alvCombo->setModel( kp()->alvTyypit() );

    ui->buttonBox->button( QDialogButtonBox::Ok )->setEnabled( false );

    connect( ui->tiliEdit, SIGNAL(textChanged(QString)), this, SLOT(tiliTaytetty()));
    connect( ui->vastatiliEdit, SIGNAL(textChanged(QString)), this, SLOT(tarkasta()));
    connect( ui->alvCombo, SIGNAL(currentIndexChanged(int)), this, SLOT(alvLajiMuuttui()));
    connect( ui->euroSpin, SIGNAL(editingFinished()), this, SLOT(laskeNetto()));
    connect( ui->nettoSpin, SIGNAL(editingFinished()), this, SLOT(laskeBrutto()));
    connect( ui->alvSpin, SIGNAL(editingFinished()), this, SLOT(laskeVerolla()));

    // Hakee tositteen tiedoista esitäytöt
    ui->pvmDate->setDate( model->pvm() );
    ui->seliteEdit->setText( model->otsikko());

    ui->tiliEdit->valitseTiliNumerolla( model->tositelaji().json()->luku("Oletustili"));

    // Jos kirjataan tiliotteelle, niin tiliotetili lukitaan vastatiliksi
    if( model->tiliotetili())
    {
        ui->vastatiliEdit->valitseTiliIdlla( model->tiliotetili());
        ui->vastatiliEdit->setEnabled(false);
    }
    else
    {
        // Suodatetaan tili valintojen mukaan
        int kirjauslaji = model->tositelaji().json()->luku("Kirjaustyyppi");
        if( kirjauslaji == TositelajiModel::OSTOLASKUT)
            ui->tiliEdit->suodataTyypilla("D.*");
        else if(kirjauslaji == TositelajiModel::MYYNTILASKUT)
            ui->tiliEdit->suodataTyypilla("C.*");

        // Jos tositelajille on määritelty oletusvastatili, esitäytetään sekin
        ui->vastatiliEdit->valitseTiliNumerolla( model->tositelaji().json()->luku("Vastatili") );
    }


}

KirjausApuriDialog::~KirjausApuriDialog()
{
    delete ui;
}

void KirjausApuriDialog::tiliTaytetty()
{
    // Jos tilillä on vastatili, niin täytetään se
    Tili tili = kp()->tilit()->tiliNumerolla(  ui->tiliEdit->valittuTilinumero() );

    if( tili.id())
    {
        ui->vastatiliEdit->valitseTiliNumerolla( tili.json()->luku("Vastatili") );
        ui->alvSpin->setValue( tili.json()->luku("AlvProsentti"));
        ui->alvCombo->setCurrentIndex( ui->alvCombo->findData( tili.json()->luku("AlvLaji") ) );

        // Näytetään kohdennus jos tulostili
        ui->kohdennusCombo->setVisible( !tili.onkoTasetili());
        ui->kohdennusLabel->setVisible( !tili.onkoTasetili());
    }
    tarkasta();
}

void KirjausApuriDialog::laskeBrutto()
{
    double netto = ui->nettoSpin->value();
    if( netto )
    {
        nettoEur = ui->nettoSpin->value();
        bruttoEur = 0;

        ui->euroSpin->setValue( (100.0 + ui->alvSpin->value()) * netto / 100.0 );
    }
    tarkasta();
}

void KirjausApuriDialog::laskeVerolla()
{
    if( nettoEur )
        ui->euroSpin->setValue( (100.0 + ui->alvSpin->value() )* nettoEur / 100.0 );
    else if(bruttoEur)
        ui->nettoSpin->setValue( (100.0 * bruttoEur ) /  (100.0 + ui->alvSpin->value())  );
}

void KirjausApuriDialog::alvLajiMuuttui()
{
    int alvlaji = ui->alvCombo->currentData().toInt();

    if(  alvlaji == AlvKoodi::MYYNNIT_BRUTTO || alvlaji == AlvKoodi::MYYNNIT_NETTO
         || alvlaji == AlvKoodi::OSTOT_BRUTTO || alvlaji==AlvKoodi::OSTOT_NETTO
         || alvlaji == AlvKoodi::RAKENNUSPALVELU_OSTO )
    {
        ui->alvSpin->setVisible(true);
        ui->alvprossaLabel->setVisible(true);
        ui->nettoSpin->setVisible(true);
        ui->nettoLabel->setVisible(true);
    }
    else
    {
        ui->alvSpin->setVisible(false);
        ui->alvlajiLabel->setVisible(false);
        ui->nettoSpin->setVisible(false);
        ui->nettoLabel->setVisible(false);
    }

}

void KirjausApuriDialog::tarkasta()
{
    ui->kirjausList->clear();

    Tili tili = kp()->tilit()->tiliNumerolla( ui->tiliEdit->valittuTilinumero() );
    Tili vastatili = kp()->tilit()->tiliNumerolla( ui->vastatiliEdit->valittuTilinumero());

    // Näytetään verot jos ollaan tulotilillä ja verovelvollisuus
    bool naytaVeroruudut = kp()->asetukset()->onko("AlvVelvollinen") && !tili.onkoTasetili() && vastatili.onkoTasetili();

    ui->alvlajiLabel->setVisible(naytaVeroruudut);
    ui->alvCombo->setVisible(naytaVeroruudut);


    // Vaaditaan molemmat tilit, silloin tehdään kirjausehdotukset
    if( ui->tiliEdit->valittuTilinumero() && ui->vastatiliEdit->valittuTilinumero() )
    {

        // 1) Tavanomainen myynti: Tulotili -> Tasetili
        if( tili.onkoTulotili() && vastatili.onkoTasetili())
        {
            teeEhdotus(  QString("Tuloa (myynti) \ntulotili \t%1 %2 \tkredit, \ntasetili \t%3 %4 \tdebet")
                         .arg(tili.numero()).arg(tili.nimi()).arg(vastatili.numero()).arg(vastatili.nimi()) , false );
        }
        else if( tili.onkoMenotili() && vastatili.onkoTasetili() )
        {
            teeEhdotus(  QString("Menoa (osto) \nmenotili  \t%1 %2 \tdebet, \ntasetili \t%3 %4 \tkredit")
                         .arg(tili.numero()).arg(tili.nimi()).arg(vastatili.numero()).arg(vastatili.nimi()) , true );
        }
        else
        {
            teeEhdotus(  QString("Siirto TILILTÄ %1 %2 kredit, TILILLE %3 %4 debet")
                         .arg(tili.numero()).arg(tili.nimi()).arg(vastatili.numero()).arg(vastatili.nimi()) , false );
            teeEhdotus(  QString("Siirto TILILTÄ %3 %4 kredit, TILILLE %1 %2 debet")
                         .arg(tili.numero()).arg(tili.nimi()).arg(vastatili.numero()).arg(vastatili.nimi()) , true );
        }

        ui->buttonBox->button( QDialogButtonBox::Ok )->setEnabled( ui->euroSpin->value() != 0 );
        ui->kirjausList->setCurrentRow(0);

    }
    else
    {
        ui->buttonBox->button( QDialogButtonBox::Ok )->setEnabled( false );
    }

}

void KirjausApuriDialog::accept()
{
    // Tehdään asiaankuuluvat kirjaukset

    // Ensivaiheessa veroton kirjaus
    VientiModel *viennit = model->vientiModel();

    QModelIndex index = model->vientiModel()->lisaaVienti();
    viennit->setData(index, ui->pvmDate->date(), VientiModel::PvmRooli);
    viennit->setData(index, ui->seliteEdit->text(), VientiModel::SeliteRooli);
    viennit->setData(index, ui->tiliEdit->valittuTilinumero(), VientiModel::TiliNumeroRooli);

    // UserRole kertoo, onko ensimmäinen tili debet
    int ekaSentit = ui->euroSpin->value() * 100;

    if( ui->kirjausList->currentItem()->data( Qt::UserRole).toBool())
        viennit->setData(index, ekaSentit, VientiModel::DebetRooli);
    else
        viennit->setData(index, ekaSentit, VientiModel::KreditRooli);

    // Kohdennus
    viennit->setData(index, ui->kohdennusCombo->currentData(KohdennusModel::IdRooli), VientiModel::KohdennusRooli);

    // TODO: Veroruudut


    index = model->vientiModel()->lisaaVienti();
    viennit->setData(index, ui->pvmDate->date(), VientiModel::PvmRooli);
    viennit->setData(index, ui->seliteEdit->text(), VientiModel::SeliteRooli);
    viennit->setData(index, ui->vastatiliEdit->valittuTilinumero(), VientiModel::TiliNumeroRooli);


    if( !ui->kirjausList->currentIndex().data(Qt::UserRole).toBool())
        viennit->setData(index, ekaSentit, VientiModel::DebetRooli);
    else
        viennit->setData(index, ekaSentit, VientiModel::KreditRooli);

    // Kohdennus
    viennit->setData(index, ui->kohdennusCombo->currentData(KohdennusModel::IdRooli), VientiModel::KohdennusRooli);


    QDialog::accept();

}

void KirjausApuriDialog::teeEhdotus(const QString &teksti, bool tiliOnDebet, const QIcon &kuvake)
{
    // Lisää kirjausehdotuslistaan ehdotuksen
    QListWidgetItem *item = new QListWidgetItem(kuvake, teksti, ui->kirjausList);
    item->setData(Qt::UserRole, tiliOnDebet);
}

void KirjausApuriDialog::laskeNetto()
{
    double brutto = ui->euroSpin->value();
    if( brutto )
    {
        bruttoEur = ui->euroSpin->value();
        nettoEur = 0;

         ui->nettoSpin->setValue( (100.0 * brutto) /  (100.0 + ui->alvSpin->value())  );
    }
    tarkasta();
}
