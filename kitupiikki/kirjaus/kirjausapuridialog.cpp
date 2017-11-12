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

#include <QDebug>

#include "kirjausapuridialog.h"
#include "ui_kirjausapuridialog.h"

KirjausApuriDialog::KirjausApuriDialog(TositeModel *tositeModel, QWidget *parent) :
    QDialog(parent),
    ui(new Ui::KirjausApuriDialog),
    model(tositeModel)
{
    ui->setupUi(this);

    // Alussa suurin valinnoista jää piiloon
    ui->nettoSpin->setVisible(false);
    ui->nettoLabel->setVisible(false);
    ui->alvCombo->setVisible(false);
    ui->alvlajiLabel->setVisible(false);
    ui->alvSpin->setVisible(false);
    ui->alvprossaLabel->setVisible(false);


    // ValintaTab ylälaidassa kirjauksen tyypin valintaan
    ui->valintaTab->addTab(QIcon(":/pic/lisaa.png"),"Tulo");
    ui->valintaTab->addTab("Meno");
    ui->valintaTab->addTab("Siirto");
    ui->valintaTab->addTab("Investointi");
    ui->valintaTab->addTab("Poismyynti");
    ui->valintaTab->setCurrentIndex(SIIRTO);

    ui->kohdennusCombo->setModel( kp()->kohdennukset() );
    ui->kohdennusCombo->setModelColumn( KohdennusModel::NIMI );

    ui->alvCombo->setModel( kp()->alvTyypit() );

    ui->ehdotusView->setModel( &ehdotus );

    ui->buttonBox->button( QDialogButtonBox::Ok )->setEnabled( false );

    connect( ui->valintaTab, SIGNAL(currentChanged(int)), this, SLOT(valilehtiVaihtui(int)));

    connect( ui->tiliEdit, SIGNAL(textChanged(QString)), this, SLOT(tiliTaytetty()));
    connect( ui->vastatiliEdit, SIGNAL(textChanged(QString)), this, SLOT(ehdota()));
    connect( ui->alvCombo, SIGNAL(currentIndexChanged(int)), this, SLOT(alvLajiMuuttui()));
    connect( ui->euroSpin, SIGNAL(editingFinished()), this, SLOT(laskeNetto()));
    connect( ui->nettoSpin, SIGNAL(editingFinished()), this, SLOT(laskeBrutto()));
    connect( ui->alvSpin, SIGNAL(editingFinished()), this, SLOT(laskeVerolla()));
    connect( ui->vaihdaNappi, SIGNAL(clicked(bool)), this, SLOT(vaihdaTilit()));

    connect(ui->seliteEdit, SIGNAL(editingFinished()), this, SLOT(ehdota()));
    connect(ui->pvmDate, SIGNAL(editingFinished()), this, SLOT(ehdota()));

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
        {
            ui->tiliEdit->suodataTyypilla("D.*");
            ui->valintaTab->setCurrentIndex(MENO);
            ui->valintaTab->setTabEnabled(TULO, false);
            ui->valintaTab->setTabEnabled(SIIRTO,false);
            ui->valintaTab->setTabEnabled(POISMYYNTI, false);
        }
        else if(kirjauslaji == TositelajiModel::MYYNTILASKUT)
        {
            ui->tiliEdit->suodataTyypilla("C.*");
            ui->valintaTab->setCurrentIndex(TULO);
            ui->valintaTab->setTabEnabled(TULO, false);
            ui->valintaTab->setTabEnabled(SIIRTO, false);
            ui->valintaTab->setTabEnabled(INVESTOINTI, false);
        }

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

        if( tili.onkoMenotili() )
        {
            ui->valintaTab->setCurrentIndex(MENO);
        }
        else if( tili.onkoTulotili() )
        {
            ui->valintaTab->setCurrentIndex(TULO);
        }
        else if( tili.onkoTasetili())
            ui->valintaTab->setCurrentIndex(SIIRTO);

        // Tilityyppi määrää, mitkä välilehdet mahdollisia!
        ui->valintaTab->setTabEnabled(MENO, tili.onkoMenotili());
        ui->valintaTab->setTabEnabled(TULO, tili.onkoTulotili());
        ui->valintaTab->setTabEnabled(SIIRTO, tili.onkoTasetili());
        ui->valintaTab->setTabEnabled(INVESTOINTI, tili.onkoMenotili() || (tili.onkoTasetili() && !tili.onkoRahaTili()));
        ui->valintaTab->setTabEnabled(POISMYYNTI, tili.onkoTulotili() || (tili.onkoTasetili() && !tili.onkoRahaTili()));

    }
    ehdota();
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
    ehdota();
}

void KirjausApuriDialog::laskeVerolla()
{
    if( nettoEur )
        ui->euroSpin->setValue( (100.0 + ui->alvSpin->value() )* nettoEur / 100.0 );
    else if(bruttoEur)
        ui->nettoSpin->setValue( (100.0 * bruttoEur ) /  (100.0 + ui->alvSpin->value())  );
    ehdota();
}

void KirjausApuriDialog::alvLajiMuuttui()
{
    int alvlaji = ui->alvCombo->currentData().toInt();

    if(  alvlaji == AlvKoodi::MYYNNIT_NETTO
         || alvlaji==AlvKoodi::OSTOT_NETTO
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
    ehdota();
}

void KirjausApuriDialog::vaihdaTilit()
{
    int aputili = ui->tiliEdit->valittuTilinumero();
    ui->tiliEdit->valitseTiliNumerolla( ui->vastatiliEdit->valittuTilinumero());
    ui->vastatiliEdit->valitseTiliNumerolla( aputili );
    ehdota();
}

void KirjausApuriDialog::ehdota()
{
    ehdotus.tyhjaa();

    Tili tili = kp()->tilit()->tiliNumerolla( ui->tiliEdit->valittuTilinumero() );
    Tili vastatili = kp()->tilit()->tiliNumerolla( ui->vastatiliEdit->valittuTilinumero());
    int nettoSnt = (int) (ui->nettoSpin->value() * 100.0);
    int bruttoSnt = (int) (ui->euroSpin->value() * 100.0);
    int alvprosentti = ui->alvSpin->value();
    int alvkoodi = ui->alvCombo->currentData(VerotyyppiModel::KoodiRooli).toInt();


    switch ( ui->valintaTab->currentIndex()) {
    case TULO:
        // Tulojen (myynti) kirjaus  per myyntitili an tasetili
        if( tili.onkoTulotili() )
        {
            VientiRivi tulorivi = uusiEhdotusRivi(tili);
            tulorivi.kreditSnt = nettoSnt;
            tulorivi.kohdennus = kp()->kohdennukset()->kohdennus(ui->kohdennusCombo->currentData(KohdennusModel::IdRooli).toInt());
            tulorivi.alvprosentti = alvprosentti;
            tulorivi.alvkoodi = alvkoodi;
            ehdotus.lisaaVienti(tulorivi);
        }
        if( vastatili.onkoTasetili() )
        {
            VientiRivi taserivi = uusiEhdotusRivi();
            taserivi.tili = vastatili;
            taserivi.debetSnt = bruttoSnt;
            ehdotus.lisaaVienti(taserivi);
        }
        break;

    case MENO:
        // Menojen (osto) kirjaus per tasetili an ostotili
        if( tili.onkoMenotili() )
        {
            VientiRivi menorivi = uusiEhdotusRivi(tili);
            menorivi.debetSnt = nettoSnt;
            menorivi.kohdennus = kp()->kohdennukset()->kohdennus(ui->kohdennusCombo->currentData(KohdennusModel::IdRooli).toInt());
            ehdotus.lisaaVienti( menorivi );
        }
        if( vastatili.onkoTasetili())
        {
            VientiRivi taserivi = uusiEhdotusRivi(vastatili);
            taserivi.kreditSnt = bruttoSnt;
            ehdotus.lisaaVienti(taserivi);
        }
        break;

    case SIIRTO:
        if( tili.onkoValidi() )
        {
            VientiRivi rivi = uusiEhdotusRivi(tili);
            rivi.debetSnt = bruttoSnt;
            ehdotus.lisaaVienti(rivi);
        }
        if( vastatili.onkoValidi())
        {
            VientiRivi rivi = uusiEhdotusRivi(tili);
            rivi.kreditSnt = bruttoSnt;
            ehdotus.lisaaVienti(rivi);
        }

        break;
    }

    ui->ehdotusView->resizeColumnsToContents();
    ui->buttonBox->button( QDialogButtonBox::Ok )->setEnabled( ehdotus.onkoKelpo() );


}

void KirjausApuriDialog::valilehtiVaihtui(int indeksi)
{
    ui->kohdennusLabel->setVisible( indeksi != SIIRTO );
    ui->kohdennusCombo->setVisible( indeksi != SIIRTO);

    ui->nimikeLabel->setVisible( indeksi==INVESTOINTI || indeksi == POISMYYNTI);
    ui->nimikeCombo->setVisible(indeksi==INVESTOINTI || indeksi == POISMYYNTI);
    ui->uusiNimikeButton->setVisible(indeksi==INVESTOINTI );

    ui->myyntitiliLabel->setVisible( indeksi == POISMYYNTI);
    ui->myyntitiliEdit->setVisible( indeksi == POISMYYNTI );

    ui->alvlajiLabel->setVisible(kp()->asetukset()->onko("AlvVelvollinen") || indeksi != SIIRTO);
    ui->alvCombo->setVisible(kp()->asetukset()->onko("AlvVelvollinen") || indeksi != SIIRTO);

    ui->vaihdaNappi->setEnabled( indeksi == SIIRTO);


    ehdota();
}

void KirjausApuriDialog::accept()
{
    ehdotus.tallenna(model->vientiModel());
    QDialog::accept();
}

VientiRivi KirjausApuriDialog::uusiEhdotusRivi(Tili tili, int debetSnt, int kreditSnt)
{
    VientiRivi rivi;
    rivi.pvm = ui->pvmDate->date();
    rivi.selite = ui->seliteEdit->text();
    rivi.tili = tili;
    rivi.debetSnt = debetSnt;
    rivi.kreditSnt = kreditSnt;
    return rivi;
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
    ehdota();
}
