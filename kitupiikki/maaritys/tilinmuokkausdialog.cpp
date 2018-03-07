/*
   Copyright (C) 2017,2018 Arto Hyvättinen

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
#include "validator/ibanvalidator.h"

TilinMuokkausDialog::TilinMuokkausDialog(TiliModel *model, QModelIndex index) :
    QDialog(), model_(model), index_(index)
{
    ui = new Ui::tilinmuokkausDialog();
    ui->setupUi(this);

    ui->numeroEdit->setValidator( new QIntValidator(0,999999999,this));
    ui->valiastiEdit->setValidator( new QIntValidator(0,999999999,this));

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
    ui->valiastiLabel->setVisible(false);
    ui->valiastiEdit->setVisible(false);

    // Tilinumeron muutosvaroitus piiloon
    ui->varoitusKuva->setVisible(false);
    ui->varoitusLabel->setVisible(false);
    ui->poistotiliEdit->asetaModel( model );
    ui->poistotiliEdit->suodataTyypilla("DP");

    ui->ibanLabel->hide();
    ui->ibanLine->hide();
    ui->ibanLine->setValidator(new IbanValidator());

    connect( ui->veroCombo, SIGNAL(currentIndexChanged(int)), this, SLOT(veroEnablePaivita()));

    connect( ui->nimiEdit, SIGNAL(textEdited(QString)), this, SLOT(tarkasta()));
    connect( ui->numeroEdit, SIGNAL(textChanged(QString)), this, SLOT(nroMuuttaaTyyppia(QString)));
    connect( ui->tyyppiCombo, SIGNAL(currentIndexChanged(int)), this, SLOT(tarkasta()));
    connect( ui->ibanLine, SIGNAL(textEdited(QString) ), this, SLOT( ibanCheck()) );

    connect( ui->otsikkoRadio, SIGNAL(clicked(bool)), this, SLOT(naytettavienPaivitys()));
    connect( ui->tiliRadio, SIGNAL(clicked(bool)), this, SLOT(naytettavienPaivitys()));

    connect( ui->valiastiEdit, SIGNAL(textChanged(QString)), this, SLOT(tarkasta()));

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
    ui->valiastiEdit->setText( QString::number(tili.json()->luku("Asti")) );

    ui->ibanLabel->setVisible( tili.onko(TiliLaji::PANKKITILI));
    ui->ibanLine->setVisible( tili.onko(TiliLaji::PANKKITILI));
    ui->ibanLine->setText(tili.json()->str("IBAN"));

    proxy_->setFilterRegExp("");
    ui->tyyppiCombo->setCurrentIndex( ui->tyyppiCombo->findData( tili.tyyppiKoodi()) );
    ui->vastatiliEdit->valitseTiliNumerolla(tili.json()->luku("Vastatili") );

    ui->veroSpin->setValue( tili.json()->luku("AlvProsentti"));

    int alvlaji = tili.json()->luku("AlvLaji");
    ui->veroCombo->setCurrentIndex( ui->veroCombo->findData( alvlaji , VerotyyppiModel::KoodiRooli) );

    ui->poistoaikaSpin->setValue( tili.json()->luku("Tasaerapoisto") / 12);  // kk -> vuosi
    ui->poistoprossaSpin->setValue( tili.json()->luku("Menojaannospoisto"));
    ui->kirjausohjeText->setPlainText( tili.json()->str("Kirjausohje"));
    ui->poistotiliEdit->valitseTiliNumerolla( tili.json()->luku("Poistotili"));

    int taseEraValinta = tili.json()->luku("Taseerittely");
    ui->taseEratRadio->setChecked( taseEraValinta == Tili::TASEERITTELY_TAYSI);
    ui->taseEraLuettelo->setChecked( taseEraValinta == Tili::TASEERITTELY_LISTA);
    ui->teLiVaRadio->setChecked( taseEraValinta == Tili::TASEERITTELY_MUUTOKSET);
    ui->teSaldoRadio->setChecked( taseEraValinta == Tili::TASEERITTELY_SALDOT);


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

    ui->ibanLabel->setVisible( tyyppi.onko(TiliLaji::PANKKITILI));
    ui->ibanLine->setVisible( tyyppi.onko(TiliLaji::PANKKITILI));

    ui->poistoaikaLabel->setVisible( tyyppi.onko( TiliLaji::TASAERAPOISTO));
    ui->poistoaikaSpin->setVisible( tyyppi.onko(TiliLaji::TASAERAPOISTO) );

    ui->poistoprossaLabel->setVisible( tyyppi.onko(TiliLaji::MENOJAANNOSPOISTO));
    ui->poistoprossaSpin->setVisible( tyyppi.onko(TiliLaji::MENOJAANNOSPOISTO ));

    ui->poistotiliLabel->setVisible( tyyppi.onko(TiliLaji::POISTETTAVA));
    ui->poistotiliEdit->setVisible( tyyppi.onko(TiliLaji::POISTETTAVA));

    // #46 Alv-velka ja alv-saatava -tileille ei voi tehdä tase-erittelyä, koska tilit tyhjennetään aina
    // kuukauden lopussa alv-kirjauksella, joka ei huomioi tase-eriä

    ui->teGroup->setVisible( tyyppi.onko(TiliLaji::TASE) && !tyyppi.onko(TiliLaji::ALVSAATAVA) && !tyyppi.onko(TiliLaji::ALVVELKA));

}

void TilinMuokkausDialog::nroMuuttaaTyyppia(const QString &nroteksti)
{
    if( !nroteksti.isEmpty())
    {
        int ekanro = nroteksti.left(1).toInt();
        int nykyluonne = ui->tyyppiCombo->currentData(TilityyppiModel::LuonneRooli).toInt();

        // Jos numero alkaa 1, pitää olla vastaavaa-tili
        // 2 pitää olla vastattavaa
        // 3-> pitää olla tulostili

        if( ekanro == 1 )
        {
            proxy_->setFilterRegExp("A.*");
            if(( nykyluonne & TiliLaji::VASTAAVAA ) != TiliLaji::VASTAAVAA )
                ui->tyyppiCombo->setCurrentIndex( ui->tyyppiCombo->findText("Vastaavaa"));
        }
        else if( ekanro == 2)
        {
            proxy_->setFilterRegExp("(B.*|T)");
            if(( nykyluonne & TiliLaji::VASTATTAVAA) != TiliLaji::VASTATTAVAA )
                ui->tyyppiCombo->setCurrentIndex(ui->tyyppiCombo->findText("Vastattavaa"));
        }
        else if( ekanro > 2 )
        {
            proxy_->setFilterRegExp("[CD].*");
            if(( nykyluonne & TiliLaji::TULOS) != TiliLaji::TULOS )
            {
                if( ekanro == 3)
                    ui->tyyppiCombo->setCurrentIndex( ui->tyyppiCombo->findText("Liikevaihtotulo (myynti)"));
                else
                    ui->tyyppiCombo->setCurrentIndex( ui->tyyppiCombo->findText("Menot"));
            }
        }

        int ysinro = Tili::ysiluku( nroteksti.toInt(), ui->tasoSpin->value() ? ui->otsikkoRadio->isChecked() : 0);
        // Haetaan ylempi otsikkoteksti

        Tili ylatili;
        for( int i=0; i < model_->rowCount(QModelIndex()) ; i++)
        {
           Tili tili = model_->tiliIndeksilla(i);
           int asti = tili.json()->luku("Asti") ? tili.json()->luku("Asti") : tili.numero();
           if( ysinro > tili.ysivertailuluku() && ysinro < Tili::ysiluku( asti, true )  )
           {
               if( tili.otsikkotaso() > ylatili.otsikkotaso() && tili.ysivertailuluku() != ysinro)
                   ylatili = tili;
           }
        }
        ui->kuuluuLabel->setText( ylatili.nimi() );

        // Vaikuttaa otsikon otsikkotasoon
        ui->tasoSpin->setValue( ylatili.otsikkotaso() == 9 ? 9 : ylatili.otsikkotaso() + 1  );


    }
    else
        ui->kuuluuLabel->clear();

    tarkasta(); // Lopuksi tarkastetaan kelpaako numero

}

void TilinMuokkausDialog::tarkasta()
{

   int luku = ui->numeroEdit->text().toInt();

   int taso = ui->tasoSpin->value();
   if( ui->tiliRadio->isChecked())
       taso = 0;

   int ysina = Tili::ysiluku(luku, taso);   // Ysivertailunumero

   if( ui->otsikkoRadio->isChecked() )
   {
       if( Tili::ysiluku( ui->valiastiEdit->text().toInt()) < ysina )
           ui->valiastiEdit->setText( ui->numeroEdit->text() );
   }

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

   // Nimen ja numeron pitää olla täytetty
   if(  !luku || ui->nimiEdit->text().isEmpty() )
   {
        ui->buttonBox->button(QDialogButtonBox::Ok)->setEnabled(false);
        return;
   }

   naytettavienPaivitys();

}

void TilinMuokkausDialog::ibanCheck()
{
    switch ( IbanValidator::kelpo( ui->ibanLine->text())) {
    case IbanValidator::Acceptable:
        ui->ibanLine->setStyleSheet("color: darkGreen;");
        break;
    case IbanValidator::Invalid :
        ui->ibanLine->setStyleSheet("color: red;");
        break;
    default:
        ui->ibanLine->setStyleSheet("color: black;");
        break;
    }
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

    if( tilityyppi.onko(TiliLaji::POISTETTAVA) && !ui->poistotiliEdit->valittuTilinumero())
    {
        QMessageBox::critical(this, tr("Tiedot puutteelliset"), tr("Tilille on määriteltävä poistojen kirjaustili."));
        return;
    }

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

    if( tilityyppi.onko(TiliLaji::PANKKITILI) && IbanValidator::kelpaako( ui->ibanLine->text() ) )
        json->set("IBAN", ui->ibanLine->text().simplified().remove(' '));
    else
        json->unset("IBAN");

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

        if( tilityyppi.onko( TiliLaji::POISTETTAVA))
            json->set("Poistotili", ui->poistotiliEdit->valittuTilinumero());
        else
            json->unset("Poistotili");

        if( tilityyppi.onko( TiliLaji::TASE))
        {
            if( ui->taseEraLuettelo->isChecked() )
                json->set("Taseerittely", Tili::TASEERITTELY_LISTA);
            else if( ui->taseEratRadio->isChecked() )
                json->set("Taseerittely", Tili::TASEERITTELY_TAYSI);
            else if( ui->teLiVaRadio->isChecked())
                json->set("Taseerittely", Tili::TASEERITTELY_MUUTOKSET);
            else
                json->unset("Taseerittely");
        }
        else
            json->unset("Taseerittely");


    }

    if( taso && ui->valiastiEdit->text().toInt() != ui->numeroEdit->text().toInt() )
        json->set("Asti",  ui->valiastiEdit->text().toInt() );
    else
        json->unset("Asti");

    if( uusitili.numero() )     // Lisätään uusi tili
        model_->lisaaTili( uusitili );


    QDialog::accept();
}

