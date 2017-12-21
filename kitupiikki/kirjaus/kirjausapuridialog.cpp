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
#include <QTimer>
#include <cmath>
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
    ui->eraLabel->setVisible(false);
    ui->taseEraCombo->setVisible(false);
    ui->vastaTaseEraLabel->setVisible(false);
    ui->vastaTaseEraCombo->setVisible(false);
    ui->poistoLabel->setVisible(false);
    ui->poistoSpin->setVisible(false);
    ui->alvVaaraKuva->setVisible(false);
    ui->alvVaaraTeksti->setVisible(false);


    // ValintaTab ylälaidassa kirjauksen tyypin valintaan
    ui->valintaTab->addTab(QIcon(":/pic/lisaa.png"),"Tulo");
    ui->valintaTab->addTab(QIcon(":/pic/poista.png"),"Meno");
    ui->valintaTab->addTab(QIcon(":/pic/siirra.png"),"Siirto");
    ui->valintaTab->setCurrentIndex(SIIRTO);

    ui->kohdennusCombo->setModel( kp()->kohdennukset() );
    ui->kohdennusCombo->setModelColumn( KohdennusModel::NIMI );

    ui->taseEraCombo->setModel( &eraModelTilille);
    ui->vastaTaseEraCombo->setModel(&eraModelVastaTilille);

    verofiltteri.setSourceModel( kp()->alvTyypit() );
    verofiltteri.setFilterRole( VerotyyppiModel::KoodiTekstiRooli);
    ui->alvCombo->setModel( &verofiltteri );

    ui->ehdotusView->setModel( &ehdotus );

    ui->buttonBox->button( QDialogButtonBox::Ok )->setEnabled( false );

    connect( ui->valintaTab, SIGNAL(currentChanged(int)), this, SLOT(valilehtiVaihtui(int)));

    connect( ui->tiliEdit, SIGNAL(editingFinished()), this, SLOT(tiliTaytetty()));

    connect( ui->vastatiliEdit, SIGNAL(textChanged(QString)), this, SLOT(vastaTiliMuuttui()));
    connect( ui->alvCombo, SIGNAL(currentIndexChanged(int)), this, SLOT(alvLajiMuuttui()));
    connect( ui->euroSpin, SIGNAL(editingFinished()), this, SLOT(laskeNetto()));
    connect( ui->nettoSpin, SIGNAL(editingFinished()), this, SLOT(laskeBrutto()));
    connect( ui->alvSpin, SIGNAL(editingFinished()), this, SLOT(laskeVerolla()));
    connect( ui->vaihdaNappi, SIGNAL(clicked(bool)), this, SLOT(ehdota()));
    connect(ui->seliteEdit, SIGNAL(editingFinished()), this, SLOT(ehdota()));
    connect(ui->pvmDate, SIGNAL(editingFinished()), this, SLOT(ehdota()));
    connect(ui->taseEraCombo, SIGNAL(currentIndexChanged(int)), this, SLOT(eraValittu()));
    connect(ui->vastaTaseEraCombo, SIGNAL(currentIndexChanged(int)), this, SLOT(eraValittu()));
    connect(ui->pvmDate, SIGNAL(dateChanged(QDate)), this, SLOT(pvmMuuttuu()));

    // Hakee tositteen tiedoista esitäytöt
    QDate pvm = model->pvm();
    ui->pvmDate->setDate( pvm );
    ui->pvmDate->setDateRange(kp()->tilikausiPaivalle(pvm).alkaa() , kp()->tilikausiPaivalle(pvm).paattyy() );
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
            ui->valintaTab->setCurrentIndex(MENO);
            ui->valintaTab->setTabEnabled(TULO, false);
            ui->valintaTab->setTabEnabled(SIIRTO,false);
        }
        else if(kirjauslaji == TositelajiModel::MYYNTILASKUT)
        {
            ui->valintaTab->setCurrentIndex(TULO);
            ui->valintaTab->setTabEnabled(MENO, false);
            ui->valintaTab->setTabEnabled(SIIRTO, false);
        }

        // Jos tositelajille on määritelty oletusvastatili, esitäytetään sekin
        ui->vastatiliEdit->valitseTiliNumerolla( model->tositelaji().json()->luku("Vastatili") );
    }
    QTimer::singleShot(0, this, SLOT(korjaaSarakeLeveydet()));

}

KirjausApuriDialog::~KirjausApuriDialog()
{
    delete ui;
}

void KirjausApuriDialog::tiliTaytetty()
{
    // Jos tilillä on vastatili, niin täytetään se
    Tili tili = kp()->tilit()->tiliNumerolla(  ui->tiliEdit->valittuTilinumero() );

    if( tili.onkoValidi() && tili.numero() && ui->tiliEdit->text().length() > 5)
    {
        if( tili.json()->luku("Vastatili")  )
        {
            ui->vastatiliEdit->valitseTiliNumerolla( tili.json()->luku("Vastatili") );
            vastaTiliMuuttui();
        }
        ui->alvSpin->setValue( tili.json()->luku("AlvProsentti"));
        ui->alvCombo->setCurrentIndex( ui->alvCombo->findData( tili.json()->luku("AlvLaji") ) );

        if( tili.onko(TiliLaji::MENO) )
        {
            ui->valintaTab->setCurrentIndex(MENO);
        }
        else if( tili.onko(TiliLaji::TULO) )
        {
            ui->valintaTab->setCurrentIndex(TULO);
        }
        else if( tili.onko(TiliLaji::TASE) && !tili.onko(TiliLaji::POISTETTAVA))
        {
            ui->valintaTab->setCurrentIndex(SIIRTO);
        }

        ui->eraLabel->setVisible( tili.eritellaankoTase() );
        ui->taseEraCombo->setVisible( tili.eritellaankoTase() );
        if( tili.eritellaankoTase() )
        {
            eraModelTilille.lataa( tili );
            ui->taseEraCombo->setCurrentIndex(0);
        }

        // Tilityyppi määrää, mitkä välilehdet mahdollisia!
        ui->valintaTab->setTabEnabled(MENO, tili.onko(TiliLaji::MENO) || tili.onko(TiliLaji::POISTETTAVA));
        ui->valintaTab->setTabEnabled(TULO, tili.onko(TiliLaji::TULO) || tili.onko(TiliLaji::POISTETTAVA));
        ui->valintaTab->setTabEnabled(SIIRTO, tili.onko(TiliLaji::TASE) );

        // Verotuksen oletuskäsittely
        ui->alvCombo->setCurrentIndex( ui->alvCombo->findData( tili.json()->luku("AlvLaji") ) );
        ui->alvSpin->setValue( tili.json()->luku("AlvProsentti"));
        valilehtiVaihtui( ui->valintaTab->currentIndex());  // Jotta veroruudut näytetään!

        // Tasaeräpoiston oletusprossa
        ui->poistoSpin->setValue( tili.json()->luku("Tasaerapoisto") / 12);  // kuukaudet -> vuodet

    }
    else
    {
        ui->valintaTab->setTabEnabled(MENO, true);
        ui->valintaTab->setTabEnabled(TULO, true);
        ui->valintaTab->setTabEnabled(SIIRTO, true );
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

    if( ui->alvCombo->currentData(VerotyyppiModel::NollaLajiRooli).toBool())
    {
        ui->alvSpin->setValue(0);
        ui->alvSpin->setEnabled(false);
        laskeNetto();
    }
    else
    {
        ui->alvSpin->setEnabled(true);
        Tili tili = kp()->tilit()->tiliNumerolla(  ui->tiliEdit->valittuTilinumero() );
        if( tili.json()->luku("AlvProsentti"))
            ui->alvSpin->setValue( tili.json()->luku("AlvProsentti"));
        else
            ui->alvSpin->setValue( VerotyyppiModel::oletusAlvProsentti());

    }

    ui->alvSpin->setVisible( alvlaji );
    ui->alvprossaLabel->setVisible(alvlaji);

    ehdota();
}

void KirjausApuriDialog::vastaTiliMuuttui()
{
    Tili vastatili = kp()->tilit()->tiliNumerolla( ui->vastatiliEdit->valittuTilinumero());
    ui->vastaTaseEraLabel->setVisible( vastatili.eritellaankoTase() );
    ui->vastaTaseEraCombo->setVisible( vastatili.eritellaankoTase() );
    if( vastatili.eritellaankoTase() )
    {
        eraModelVastaTilille.lataa( vastatili );
        ui->vastaTaseEraCombo->setCurrentIndex(0);
    }
    ehdota();
}

void KirjausApuriDialog::eraValittu()
{
    // Kun tase-erä valitaan, merkitään summaksi oletuksena kyseisen tase-erän saldo
    if( !ui->euroSpin->value())
        ui->euroSpin->setValue( qAbs( ui->taseEraCombo->currentData( EranValintaModel::SaldoRooli).toDouble() / 100.0  ) );
    laskeNetto();
    ehdota();
}

void KirjausApuriDialog::vastaEraValittu()
{
    if( !ui->euroSpin->value())
        ui->euroSpin->setValue( qAbs( ui->vastaTaseEraCombo->currentData( EranValintaModel::SaldoRooli).toDouble() / 100.0  ) );
    laskeNetto();
    ehdota();
}

void KirjausApuriDialog::pvmMuuttuu()
{
    // Jos alvit jo ilmoitettu, ei voi ilmoittaa alvillista
    if(  kp()->asetukset()->onko("AlvVelvollinen") )
    {
        bool alvlukko =  kp()->asetukset()->pvm("AlvIlmoitus").daysTo( ui->pvmDate->date() ) < 1 ;

        ui->alvVaaraKuva->setVisible(alvlukko);
        ui->alvVaaraTeksti->setVisible(alvlukko);
        ui->alvCombo->setEnabled(!alvlukko);
        if( alvlukko )
            ui->alvCombo->setCurrentIndex(0);
    }
}

void KirjausApuriDialog::ehdota()
{
    ehdotus.tyhjaa();

    Tili tili = kp()->tilit()->tiliNumerolla( ui->tiliEdit->valittuTilinumero() );
    Tili vastatili = kp()->tilit()->tiliNumerolla( ui->vastatiliEdit->valittuTilinumero());
    int nettoSnt = std::round(ui->nettoSpin->value() * 100.0);
    int bruttoSnt = std::round(ui->euroSpin->value() * 100.0);
    int alvprosentti = ui->alvSpin->value();
    int alvkoodi = ui->alvCombo->currentData(VerotyyppiModel::KoodiRooli).toInt();


    switch ( ui->valintaTab->currentIndex()) {
    case TULO:
        // Tulojen (myynti) kirjaus  per myyntitili an tasetili
        if( tili.onko(TiliLaji::TULO)  || tili.onko( TiliLaji::POISTETTAVA))
        {
            VientiRivi tulorivi = uusiEhdotusRivi(tili);
            if( alvkoodi == AlvKoodi::MYYNNIT_BRUTTO)
                tulorivi.kreditSnt = bruttoSnt;
            else
                tulorivi.kreditSnt = nettoSnt;
            tulorivi.kohdennus = kp()->kohdennukset()->kohdennus(ui->kohdennusCombo->currentData(KohdennusModel::IdRooli).toInt());
            tulorivi.alvprosentti = alvprosentti;
            tulorivi.alvkoodi = alvkoodi;
            tulorivi.eraId = ui->taseEraCombo->currentData(EranValintaModel::EraIdRooli).toInt();
            ehdotus.lisaaVienti(tulorivi);
        }
        if(alvkoodi == AlvKoodi::MYYNNIT_NETTO && kp()->tilit()->tiliTyypilla(TiliLaji::ALVVELKA).onkoValidi() )
        {

            VientiRivi verorivi = uusiEhdotusRivi( kp()->tilit()->tiliTyypilla(TiliLaji::ALVVELKA));
            verorivi.kreditSnt = bruttoSnt - nettoSnt;
            verorivi.alvprosentti = alvprosentti;
            verorivi.alvkoodi = AlvKoodi::ALVKIRJAUS + alvkoodi;
            ehdotus.lisaaVienti(verorivi);
        }
        if( vastatili.onko(TiliLaji::TASE) )
        {
            VientiRivi taserivi = uusiEhdotusRivi();
            taserivi.tili = vastatili;
            if( alvkoodi == AlvKoodi::MYYNNIT_NETTO || alvkoodi == AlvKoodi::MYYNNIT_BRUTTO )
                taserivi.debetSnt = bruttoSnt;
            else
                taserivi.debetSnt = nettoSnt;
            taserivi.eraId = ui->vastaTaseEraCombo->currentData(EranValintaModel::EraIdRooli).toInt();
            ehdotus.lisaaVienti(taserivi);
        }
        break;

    case MENO:
        // Menojen (osto) kirjaus per tasetili an ostotili
        if( tili.onko(TiliLaji::MENO) || tili.onko(TiliLaji::POISTETTAVA)  )
        {
            VientiRivi menorivi = uusiEhdotusRivi(tili);
            if( alvkoodi == AlvKoodi::OSTOT_BRUTTO)
                menorivi.debetSnt = bruttoSnt;
            else
                menorivi.debetSnt = nettoSnt;

            menorivi.kohdennus = kp()->kohdennukset()->kohdennus(ui->kohdennusCombo->currentData(KohdennusModel::IdRooli).toInt());
            menorivi.alvprosentti = alvprosentti;
            menorivi.alvkoodi = alvkoodi;
            menorivi.eraId = ui->taseEraCombo->currentData(EranValintaModel::EraIdRooli).toInt();
            if(tili.tyyppi().onko(TiliLaji::TASAERAPOISTO))
            {
                menorivi.json.set("Tasaerapoisto", ui->poistoSpin->value() * 12);  // vuodet -> kk
            }

            ehdotus.lisaaVienti( menorivi );

        }
        if( alvprosentti && kp()->tilit()->tiliTyypilla(TiliLaji::ALVSAATAVA).onkoValidi() &&
                alvkoodi != AlvKoodi::OSTOT_BRUTTO)
        {
            VientiRivi verorivi = uusiEhdotusRivi( kp()->tilit()->tiliTyypilla(TiliLaji::ALVSAATAVA) );
            verorivi.debetSnt = bruttoSnt - nettoSnt;
            verorivi.alvprosentti = alvprosentti;
            verorivi.alvkoodi = AlvKoodi::ALVVAHENNYS + alvkoodi;
            ehdotus.lisaaVienti(verorivi);

            if( (alvkoodi == AlvKoodi::YHTEISOHANKINNAT_PALVELUT || alvkoodi==AlvKoodi::YHTEISOHANKINNAT_TAVARAT ||
                alvkoodi == AlvKoodi::RAKENNUSPALVELU_OSTO) && kp()->tilit()->tiliTyypilla(TiliLaji::ALVVELKA).onkoValidi() )
            {
                VientiRivi lisarivi = uusiEhdotusRivi( kp()->tilit()->tiliTyypilla(TiliLaji::ALVVELKA));
                lisarivi.kreditSnt = bruttoSnt - nettoSnt;
                lisarivi.alvprosentti = alvprosentti;
                lisarivi.alvkoodi = AlvKoodi::ALVKIRJAUS + alvkoodi;
                ehdotus.lisaaVienti(lisarivi);
            }
        }
        if( vastatili.onko(TiliLaji::TASE))
        {
            VientiRivi taserivi = uusiEhdotusRivi(vastatili);
            if( alvkoodi == AlvKoodi::OSTOT_NETTO || alvkoodi == AlvKoodi::OSTOT_BRUTTO)
                taserivi.kreditSnt = bruttoSnt;
            else
                taserivi.kreditSnt = nettoSnt;
            taserivi.eraId = ui->vastaTaseEraCombo->currentData(EranValintaModel::EraIdRooli).toInt();
            ehdotus.lisaaVienti(taserivi);
        }
        break;

    case SIIRTO:
        if( tili.onkoValidi() )
        {
            VientiRivi rivi = uusiEhdotusRivi(tili);

            if( ui->vaihdaNappi->isChecked())
                rivi.kreditSnt = bruttoSnt;
            else
                rivi.debetSnt = bruttoSnt;

            rivi.eraId = ui->taseEraCombo->currentData(EranValintaModel::EraIdRooli).toInt();
            ehdotus.lisaaVienti(rivi);
        }
        if( vastatili.onkoValidi())
        {
            VientiRivi rivi = uusiEhdotusRivi(vastatili);
            if( ui->vaihdaNappi->isChecked())
                rivi.debetSnt = bruttoSnt;
            else
                rivi.kreditSnt = bruttoSnt;
            rivi.eraId = ui->vastaTaseEraCombo->currentData(EranValintaModel::EraIdRooli).toInt();
            ehdotus.lisaaVienti(rivi);
        }

        break;
    }

    // Netto näytetään jos vero
    ui->nettoLabel->setVisible(ui->alvSpin->value());
    ui->nettoSpin->setVisible( ui->alvSpin->value());

    ui->buttonBox->button( QDialogButtonBox::Ok )->setEnabled( ehdotus.onkoKelpo() );

    // Poisto näytetään jos kirjataan tasaeräpoistotilille
    ui->poistoLabel->setVisible( tili.onko(TiliLaji::TASAERAPOISTO) && !ui->taseEraCombo->currentIndex());
    ui->poistoSpin->setVisible(tili.onko(TiliLaji::TASAERAPOISTO) && !ui->taseEraCombo->currentIndex());

}


void KirjausApuriDialog::valilehtiVaihtui(int indeksi)
{
    ui->kohdennusLabel->setVisible( indeksi != SIIRTO && kp()->kohdennukset()->rowCount(QModelIndex()) > 1);
    ui->kohdennusCombo->setVisible( indeksi != SIIRTO && kp()->kohdennukset()->rowCount(QModelIndex()) > 1);
    ui->vaihdaNappi->setVisible(indeksi == SIIRTO );


    bool verot = kp()->asetukset()->onko("AlvVelvollinen") && indeksi != SIIRTO;

    ui->alvlajiLabel->setVisible(verot);
    ui->alvCombo->setVisible(verot);
    // Ensimmäisenä combossa verottomuus
    ui->alvprossaLabel->setVisible(verot && ui->alvCombo->currentIndex() > 0);
    ui->alvSpin->setVisible(verot && ui->alvCombo->currentIndex() > 0 );

    if(!verot)
        ui->alvSpin->setValue(0);


    if( indeksi == MENO )
    {
        verofiltteri.setFilterRegExp("(0|2.)");
        ui->tiliEdit->suodataTyypilla("(AP|D).*");
        ui->vastatiliEdit->suodataTyypilla("[AB].*");
    }
    else if( indeksi == TULO)
    {
        verofiltteri.setFilterRegExp("(0|1.)");
        ui->tiliEdit->suodataTyypilla("(AP|C).*");
        ui->vastatiliEdit->suodataTyypilla("[AB].*");
    }
    else
    {
        ui->tiliEdit->suodataTyypilla("[ABCD].*");
        ui->vastatiliEdit->suodataTyypilla("[ABCD].*");
    }

    if( indeksi == MENO)
        ui->tiliLabel->setText("Menotili");
    else if(indeksi == TULO)
        ui->tiliLabel->setText("Tulotili");
    else if(indeksi == SIIRTO)
        ui->tiliLabel->setText("Debet-tili");

    if( indeksi == SIIRTO)
        ui->vastatiliLabel->setText("Kredit-tili");
    else
        ui->vastatiliLabel->setText("Vastatili");

    ehdota();
}

void KirjausApuriDialog::korjaaSarakeLeveydet()
{
    ui->ehdotusView->setColumnWidth( EhdotusModel::TILI, ui->ehdotusView->width() - ui->ehdotusView->columnWidth(EhdotusModel::KREDIT)
                                     - ui->ehdotusView->columnWidth(EhdotusModel::DEBET) - 10);
    ui->tiliEdit->setFocus();
}

void KirjausApuriDialog::accept()
{
    ehdota();
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
