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

#include <QMapIterator>
#include <QIntValidator>

#include <QDebug>
#include <QPushButton>
#include <QMessageBox>
#include <QIntValidator>

#include "tilinmuokkausdialog.h"
#include "db/tilimodel.h"
#include "db/tilinvalintaline.h"

TilinMuokkausDialog::TilinMuokkausDialog(TiliModel *model, QModelIndex index) :
    QDialog(), model_(model), index_(index)
{
    ui = new Ui::tilinmuokkausDialog();
    ui->setupUi(this);

    ui->numeroEdit->setValidator( new QIntValidator(0,999999999,this));

    ui->vastatiliEdit->asetaModel( model );

    proxy_ = new QSortFilterProxyModel(this);
    proxy_->setSourceModel( kp()->tiliTyypit() );
    proxy_->setFilterRole(TilityyppiModel::KoodiRooli);
    ui->tyyppiCombo->setModel( proxy_ );

    // Laitetaa verotyypit paikalleen

    veroproxy_ = new QSortFilterProxyModel(this);
    veroproxy_->setSourceModel( kp()->alvTyypit());
    veroproxy_->setFilterRole( VerotyyppiModel::KoodiRooli);
    ui->veroCombo->setModel( veroproxy_ );

    // Vain otsikkoon liittyvät piilotetaan
    ui->tasoSpin->setVisible(false);
    ui->tasoLabel->setVisible(false);

    // Tilinumeron muutosvaroitus piiloon
    ui->varoitusKuva->setVisible(false);
    ui->varoitusLabel->setVisible(false);

    connect( ui->veroCombo, SIGNAL(currentIndexChanged(int)), this, SLOT(veroEnablePaivita()));
    connect( ui->numeroEdit, SIGNAL(textChanged(QString)), this, SLOT(otsikkoTasoPaivita()));

    connect( ui->nimiEdit, SIGNAL(textEdited(QString)), this, SLOT(tarkasta()));
    connect( ui->numeroEdit, SIGNAL(textEdited(QString)), this, SLOT(nroMuuttaaTyyppia(QString)));
    connect( ui->tyyppiCombo, SIGNAL(currentIndexChanged(int)), this, SLOT(tarkasta()));

    connect( ui->otsikkoRadio, SIGNAL(clicked(bool)), this, SLOT(naytettavienPaivitys()));
    connect( ui->tiliRadio, SIGNAL(clicked(bool)), this, SLOT(naytettavienPaivitys()));

    // Tallennusnappi ei käytössä ennen kuin tiedot kunnossa
    ui->buttonBox->button(QDialogButtonBox::Ok)->setEnabled(false);

    naytettavienPaivitys();

    if( index.isValid())
        lataa();

}

TilinMuokkausDialog::~TilinMuokkausDialog()
{
    delete ui;
}

void TilinMuokkausDialog::lataa()
{
    Tili tili = model_->tiliIndeksilla( index_.row());

    // Ei voi muuttaa otsikkoa tiliksi tai päin vastoin
    ui->tiliRadio->setEnabled(false);
    ui->otsikkoRadio->setEnabled(false);

    ui->tiliRadio->setChecked( tili.otsikkotaso() == 0);
    ui->otsikkoRadio->setChecked( tili.otsikkotaso() );

    ui->nimiEdit->setText( tili.nimi());
    ui->taydentavaEdit->setText( tili.json()->str("Taydentava"));
    ui->numeroEdit->setText( QString::number( tili.numero()));
    ui->tasoSpin->setValue( tili.otsikkotaso());

    proxy_->setFilterRegExp("");
    ui->tyyppiCombo->setCurrentIndex( ui->tyyppiCombo->findData( tili.tyyppiKoodi()) );
    ui->vastatiliEdit->valitseTiliNumerolla(tili.json()->luku("Vastatili") );

    ui->veroSpin->setValue( tili.json()->luku("AlvProsentti"));

    int alvlaji = tili.json()->luku("AlvLaji");
    ui->veroCombo->setCurrentIndex( ui->veroCombo->findData( alvlaji , VerotyyppiModel::KoodiRooli) );

    ui->poistoaikaSpin->setValue( tili.json()->luku("Tasaerapoisto") / 12);  // kk -> vuosi
    ui->poistoprossaSpin->setValue( tili.json()->luku("Menojaannospoisto"));
    ui->kirjausohjeText->setPlainText( tili.json()->str("Kirjausohje"));

    int taseEraValinta = tili.json()->luku("Taseerittely");
    ui->taseEratRadio->setChecked( taseEraValinta == 3);
    ui->teLiVaRadio->setChecked( taseEraValinta == 2);
    ui->teSaldoRadio->setChecked( taseEraValinta == 1);
    ui->teEiRadio->setChecked( taseEraValinta == 0);

    nroMuuttaaTyyppia(QString::number( tili.numero() ));

}

void TilinMuokkausDialog::veroEnablePaivita()
{
    // Jos veroton, niin eipä silloin laiteta alv-prosenttia
    if( ui->veroCombo->currentData(VerotyyppiModel::NollaLajiRooli).toBool() )
    {
        ui->veroSpin->setEnabled(false);
    }
    else
    {
        ui->veroSpin->setEnabled(true);
        // Lisäksi laitetaan oletusvero jo nolla
        if( ui->veroSpin->value() == 0)
            ui->veroSpin->setValue( VerotyyppiModel::oletusAlvProsentti());
    }

}

void TilinMuokkausDialog::otsikkoTasoPaivita()
{
    if( ui->numeroEdit->text().toInt())
    {
        // Mahdollinen otsikkotaso on vain yksi enemmän kuin edellinen otsikkotaso
        int isoinluku = 1;
        int ysilukuna = Tili::ysiluku( ui->numeroEdit->text().toInt());

        for( int i = 0; i < model_->rowCount(QModelIndex()); i++)
        {
            Tili tili = model_->tiliIndeksilla(i);
            if( tili.ysivertailuluku() >= ysilukuna)
                // Tämän tilin paikka löydetty, eli tässä ollaan!
                break;
            if( tili.otsikkotaso() )
                isoinluku = tili.otsikkotaso() + 1;
        }
        ui->tasoSpin->setMaximum( isoinluku );
    }
}

void TilinMuokkausDialog::naytettavienPaivitys()
{

    TiliTyyppi tyyppi = kp()->tiliTyypit()->tyyppiKoodilla( ui->tyyppiCombo->currentData().toString() );
    if( ui->otsikkoRadio->isChecked() )
        tyyppi = TiliTyyppi();

    // Ellei alv-toimintoja käytettävissä, ne piilotetaan
    bool alvKaytossa = kp()->asetukset()->onko("AlvVelvollinen") &&
            ( tyyppi.onko(TiliLaji::TULOS) || tyyppi.onko(TiliLaji::POISTETTAVA));

    ui->verolajiLabel->setVisible( alvKaytossa );
    ui->veroCombo->setVisible( alvKaytossa );
    ui->veroprosenttiLabel->setVisible( alvKaytossa );
    ui->veroSpin->setVisible( alvKaytossa);

    if( tyyppi.onko(TiliLaji::TULO))
        veroproxy_->setFilterRegExp("(0|1.)");
    else if( tyyppi.onko(TiliLaji::MENO))
        veroproxy_->setFilterRegExp("(0|2.)");
    else
        veroproxy_->setFilterRegExp("");

    ui->poistoaikaLabel->setVisible( tyyppi.onko( TiliLaji::TASAERAPOISTO));
    ui->poistoaikaSpin->setVisible( tyyppi.onko(TiliLaji::TASAERAPOISTO) );

    ui->poistoprossaLabel->setVisible( tyyppi.onko(TiliLaji::MENOJAANNOSPOISTO));
    ui->poistoprossaSpin->setVisible( tyyppi.onko(TiliLaji::MENOJAANNOSPOISTO ));


    ui->teGroup->setVisible( tyyppi.onko(TiliLaji::TASE) && !tyyppi.onko(TiliLaji::MENOJAANNOSPOISTO));
    ui->taseEratRadio->setEnabled( !tyyppi.onko(TiliLaji::MENOJAANNOSPOISTO));

}

void TilinMuokkausDialog::nroMuuttaaTyyppia(const QString &nroteksti)
{
    if( !nroteksti.isEmpty())
    {
        int ekanro = nroteksti.left(1).toInt();

        // Jos numero alkaa 1, pitää olla vastaavaa-tili
        // 2 pitää olla vastattavaa
        // 3-> pitää olla tulostili

        if( ekanro == 1 )
        {
            proxy_->setFilterRegExp("A.*");
            if(( ui->tyyppiCombo->currentData(TilityyppiModel::LuonneRooli).toInt() & TiliLaji::VASTAAVAA ) != TiliLaji::VASTAAVAA )
                ui->tyyppiCombo->setCurrentIndex(0);
        }
        else if( ekanro == 2)
        {
            proxy_->setFilterRegExp("B.*");
            if(( ui->tyyppiCombo->currentData(TilityyppiModel::LuonneRooli).toInt() & TiliLaji::VASTATTAVAA) != TiliLaji::VASTATTAVAA )
                ui->tyyppiCombo->setCurrentIndex(0);
        }
        else if( ekanro == 3 )
        {
            proxy_->setFilterRegExp("[CD].*");
            if(( ui->tyyppiCombo->currentData(TilityyppiModel::LuonneRooli).toInt() & TiliLaji::TULOS) != TiliLaji::TULOS )
                ui->tyyppiCombo->setCurrentIndex(0);
        }

    }
    tarkasta(); // Lopuksi tarkastetaan kelpaako numero

}

void TilinMuokkausDialog::tarkasta()
{

   int luku = ui->numeroEdit->text().toInt();

   // Nimen ja numeron pitää olla täytetty
   if(  !luku || ui->nimiEdit->text().isEmpty() )
   {
        ui->buttonBox->button(QDialogButtonBox::Ok)->setEnabled(false);
        return;
   }
   int taso = ui->tasoSpin->value();
   if( ui->tiliRadio->isChecked())
       taso = 0;

   int ysina = Tili::ysiluku(luku, taso);   // Ysivertailunumero

   // Jos numero vaihtuu, näytetään siitä varoitus
   // Tämä siksi, että monet määritykset liittyvät tilin numeroon
   if( ui->tiliRadio->isChecked() &&
           index_.data(TiliModel::IdRooli).toInt() > 0 &&
           index_.data(TiliModel::NroRooli).toInt() != luku )
   {
       ui->varoitusKuva->setVisible(true);
       ui->varoitusLabel->setVisible(true);
   }
   else
   {
       ui->varoitusKuva->setVisible(false);
       ui->varoitusLabel->setVisible(false);
   }


   // Tarkastetaan, ettei numero ole tupla
   if( ysina != Tili::ysiluku(index_.data(TiliModel::NroRooli).toInt(), taso) )
   {
       for( int i = 0; i < model_->rowCount(QModelIndex()); i++)
           if( model_->tiliIndeksilla(i).ysivertailuluku() == ysina)
           {
               // Sama numero on jo käytössä, ei siis kelpaa!
               ui->numeroEdit->setStyleSheet("color: red;");
               ui->buttonBox->button(QDialogButtonBox::Ok)->setEnabled(false);
               return;
           }
   }
   // Ei löytynyt samaa
   ui->numeroEdit->setStyleSheet("color: black;");
   ui->buttonBox->button(QDialogButtonBox::Ok)->setEnabled(true);

   naytettavienPaivitys();

}

void TilinMuokkausDialog::accept()
{
    ui->buttonBox->setFocus();

    // Kaikki kunnossa eli voidaan tallentaa modeliin
    QString tyyppikoodi = ui->tyyppiCombo->currentData().toString();
    int taso = ui->tasoSpin->value();

    if( ui->otsikkoRadio->isChecked())
        tyyppikoodi = QString("H%1").arg(ui->tasoSpin->value());
    else
        taso = 0;

    TiliTyyppi tilityyppi = kp()->tiliTyypit()->tyyppiKoodilla(tyyppikoodi);

    JsonKentta *json;
    Tili uusitili;
    if( !index_.isValid())
    {
        // Uusi tili
        uusitili.asetaNumero(ui->numeroEdit->text().toInt());
        uusitili.asetaNimi( ui->nimiEdit->text());
        uusitili.asetaTyyppi( tyyppikoodi );

        json = uusitili.json();

    }
    else
    {
        // Päivitetään tili modeliin
        model_->setData(index_, ui->numeroEdit->text().toInt(), TiliModel::NroRooli);
        model_->setData(index_, ui->nimiEdit->text(), TiliModel::NimiRooli);
        model_->setData(index_, tyyppikoodi, TiliModel::TyyppiRooli);
        json = model_->jsonIndeksilla( index_.row());
    }

    json->set("Taydentava", ui->taydentavaEdit->text());
    json->set("Kirjausohje", ui->kirjausohjeText->toPlainText());

    if( !taso )
    {

        // Tilistä kirjoitetaan json-kentät

        if( ui->vastatiliEdit->valittuTilinumero() )
            json->set("Vastatili", ui->vastatiliEdit->valittuTilinumero());
        else
            json->unset("Vastatili");

        json->set("AlvLaji", ui->veroCombo->currentData(VerotyyppiModel::KoodiRooli).toInt());

        if( ui->veroCombo->currentData(VerotyyppiModel::NollaLajiRooli).toBool())
            json->unset("AlvProsentti");
        else
            json->set("AlvProsentti", ui->veroSpin->value());

        if( tilityyppi.onko( TiliLaji::TASAERAPOISTO))
            json->set("Tasaerapoisto", ui->poistoaikaSpin->value() * 12); // vuosi -> kk
        else
            json->unset("Tasaerapoisto");

        if( tilityyppi.onko( TiliLaji::MENOJAANNOSPOISTO))
            json->set("Menojaannospoisto", ui->poistoprossaSpin->value());
        else
            json->unset("Menojaannospoisto");

        if( tilityyppi.onko( TiliLaji::TASE))
        {
            if( ui->taseEratRadio->isChecked() || tilityyppi.onko(TiliLaji::TASAERAPOISTO))
                json->set("Taseerittely",3);
            else if( ui->teLiVaRadio->isChecked())
                json->set("Taseerittely",2);
            else if( ui->teSaldoRadio->isChecked())
                json->set("Taseerittely",1);
            else
                json->unset("Taseerittely");
        }
        else
            json->unset("Taseerittely");


    }

    if( uusitili.numero() )     // Lisätään uusi tili
        model_->lisaaTili( uusitili );


    QDialog::accept();
}

