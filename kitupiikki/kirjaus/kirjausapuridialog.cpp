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

    ui->nettoSpin->setVisible(false);
    ui->nettoLabel->setVisible(false);
    ui->alvCombo->setVisible(false);
    ui->alvlajiLabel->setVisible(false);
    ui->alvSpin->setVisible(false);
    ui->alvprossaLabel->setVisible(false);
    ui->kohdennusLabel->setVisible(false);
    ui->kohdennusCombo->setVisible(false);

    ui->nimikeLabel->setVisible(false);
    ui->nimikeCombo->setVisible(false);
    ui->uusiNimikeButton->setVisible(false);

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

    ui->buttonBox->button( QDialogButtonBox::Ok )->setEnabled( false );

    connect( ui->tiliEdit, SIGNAL(textChanged(QString)), this, SLOT(tiliTaytetty()));
    connect( ui->vastatiliEdit, SIGNAL(textChanged(QString)), this, SLOT(tarkasta()));
    connect( ui->alvCombo, SIGNAL(currentIndexChanged(int)), this, SLOT(alvLajiMuuttui()));
    connect( ui->euroSpin, SIGNAL(editingFinished()), this, SLOT(laskeNetto()));
    connect( ui->nettoSpin, SIGNAL(editingFinished()), this, SLOT(laskeBrutto()));
    connect( ui->alvSpin, SIGNAL(editingFinished()), this, SLOT(laskeVerolla()));
    connect( ui->vaihdaNappi, SIGNAL(clicked(bool)), this, SLOT(vaihdaTilit()));

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
            ui->valintaTab->setCurrentIndex(TULO);
            ui->valintaTab->setTabEnabled(MENO, false);
            ui->valintaTab->setTabEnabled(SIIRTO,false);
            ui->valintaTab->setTabEnabled(INVESTOINTI, false);
        }
        else if(kirjauslaji == TositelajiModel::MYYNTILASKUT)
        {
            ui->tiliEdit->suodataTyypilla("C.*");
            ui->valintaTab->setCurrentIndex(MENO);
            ui->valintaTab->setTabEnabled(MENO, false);
            ui->valintaTab->setTabEnabled(SIIRTO, false);
            ui->valintaTab->setTabEnabled(POISMYYNTI, false);
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

}

void KirjausApuriDialog::vaihdaTilit()
{
    int aputili = ui->tiliEdit->valittuTilinumero();
    ui->tiliEdit->valitseTiliNumerolla( ui->vastatiliEdit->valittuTilinumero());
    ui->vastatiliEdit->valitseTiliNumerolla( aputili );
}

void KirjausApuriDialog::tarkasta()
{
    // ui->kirjausList->clear();

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

            ehdotus.clear();

            VientiRivi rivi;
            rivi.pvm = ui->pvmDate->date();
            rivi.tili=tili;
            rivi.selite = ui->seliteEdit->text();
            rivi.kreditSnt = (int) (ui->nettoSpin->value() * 100.0);
            ehdotus.append(rivi);

            rivi.tili = vastatili;
            rivi.debetSnt = rivi.kreditSnt;
            rivi.kreditSnt = 0.0;
            ehdotus.append(rivi);
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
       //  ui->kirjausList->setCurrentRow(0);

    }
    else
    {
        ui->buttonBox->button( QDialogButtonBox::Ok )->setEnabled( false );
    }

}

void KirjausApuriDialog::accept()
{

    Tili tili = kp()->tilit()->tiliNumerolla( ui->tiliEdit->valittuTilinumero() );
    Tili vastatili = kp()->tilit()->tiliNumerolla( ui->vastatiliEdit->valittuTilinumero());

    // Tehdään asiaankuuluvat kirjaukset

    // Ensivaiheessa veroton kirjaus
    VientiModel *viennit = model->vientiModel();

    QModelIndex index = model->vientiModel()->lisaaVienti();
    viennit->setData(index, ui->pvmDate->date(), VientiModel::PvmRooli);
    viennit->setData(index, ui->seliteEdit->text(), VientiModel::SeliteRooli);
    viennit->setData(index, tili.numero() , VientiModel::TiliNumeroRooli);

    // UserRole kertoo, onko ensimmäinen tili debet
    int ekaSentit = ui->euroSpin->value() * 100;

    if( /* ui->kirjausList->currentItem()->data( Qt::UserRole).toBool() */ true)
        viennit->setData(index, ekaSentit, VientiModel::DebetRooli);
    else
        viennit->setData(index, ekaSentit, VientiModel::KreditRooli);

    // Kohdennus (meno- tai tulotiliin)
    if( tili.onkoMenotili() || tili.onkoTulotili())
        viennit->setData(index, ui->kohdennusCombo->currentData(KohdennusModel::IdRooli), VientiModel::KohdennusRooli);

    // TODO: Veroruudut


    index = model->vientiModel()->lisaaVienti();
    viennit->setData(index, ui->pvmDate->date(), VientiModel::PvmRooli);
    viennit->setData(index, ui->seliteEdit->text(), VientiModel::SeliteRooli);
    viennit->setData(index, vastatili.numero() , VientiModel::TiliNumeroRooli);


    if( /* !ui->kirjausList->currentIndex().data(Qt::UserRole).toBool() */ true )
        viennit->setData(index, ekaSentit, VientiModel::DebetRooli);
    else
        viennit->setData(index, ekaSentit, VientiModel::KreditRooli);

    // Kohdennus vain tulo- tai menotiliin
    if( vastatili.onkoMenotili() || vastatili.onkoTulotili())
        viennit->setData(index, ui->kohdennusCombo->currentData(KohdennusModel::IdRooli), VientiModel::KohdennusRooli);


    QDialog::accept();

}

void KirjausApuriDialog::teeEhdotus(const QString &teksti, bool tiliOnDebet, const QIcon &kuvake)
{
    // Lisää kirjausehdotuslistaan ehdotuksen
    // QListWidgetItem *item = new QListWidgetItem(kuvake, teksti, ui->kirjausList);
    // item->setData(Qt::UserRole, tiliOnDebet);
    ui->esikatseluBrowser->setHtml(teksti);

    QString txt = "<table><tr><th>Tili</th><th>Debet</th><th>Kredit</th></tr>";
    foreach (VientiRivi rivi, ehdotus) {
        txt.append( QString("<tr><td>%1</td><td>%2</td><td>%3</td></tr>").arg(rivi.tili.nimi()).arg((double)rivi.debetSnt / 100.0,0,'f',2).arg((double)rivi.kreditSnt / 100.0,0,'f',2) );
    }
    txt.append("</table>");
    ui->esikatseluBrowser->append( txt );

    ui->taulukko->clear();
    ui->taulukko->setRowCount(ehdotus.count());
    int i=0;
    foreach (VientiRivi rivi, ehdotus) {
        ui->taulukko->setItem(i,0, new QTableWidgetItem( rivi.tili.nimi() ));
        if( rivi.debetSnt )
            ui->taulukko->setItem( i,1, new QTableWidgetItem(  QString("%1").arg((double) rivi.debetSnt / 100.0,0, 'f', 2 ) ));
        if( rivi.kreditSnt )
            ui->taulukko->setItem( i, 2, new QTableWidgetItem( QString("%1").arg((double) rivi.kreditSnt / 100.0,0, 'f', 2 ) ));
        i++;

    }

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
