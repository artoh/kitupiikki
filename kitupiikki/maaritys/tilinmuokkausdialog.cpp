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

    // Laitetaan tyyppivaihtoehdot paikalleen
    QMapIterator<QString,QString> iter( model_->tiliTyyppiTaulu() );
    while( iter.hasNext() )
    {
        iter.next();
        ui->tyyppiCombo->addItem( QIcon(),iter.value(), iter.key());
    }

    // Laitetaa verotyypit paikalleen

    ui->veroCombo->setModel( kp()->alvTyypit());

    // Vain otsikkoon liittyvät piilotetaan
    ui->tasoSpin->setVisible(false);
    ui->tasoLabel->setVisible(false);

    // Tilinumeron muutosvaroitus piiloon
    ui->varoitusKuva->setVisible(false);
    ui->varoitusLabel->setVisible(false);

    // Ellei alv-toimintoja käytettävissä, ne piilotetaan
    bool alvKaytossa = kp()->asetukset()->onko("AlvVelvollinen");
    ui->verolajiLabel->setVisible( alvKaytossa );
    ui->veroCombo->setVisible( alvKaytossa );
    ui->veroprosenttiLabel->setVisible( alvKaytossa );
    ui->veroSpin->setVisible( alvKaytossa);


    connect( ui->veroCombo, SIGNAL(currentIndexChanged(int)), this, SLOT(veroEnablePaivita()));
    connect( ui->numeroEdit, SIGNAL(textChanged(QString)), this, SLOT(otsikkoTasoPaivita()));

    connect( ui->nimiEdit, SIGNAL(textEdited(QString)), this, SLOT(tarkasta()));
    connect( ui->numeroEdit, SIGNAL(textEdited(QString)), this, SLOT(nroMuuttaaTyyppia(QString)));
    connect( ui->tyyppiCombo, SIGNAL(currentIndexChanged(int)), this, SLOT(tarkasta()));

    // Tallennusnappi ei käytössä ennen kuin tiedot kunnossa
    ui->buttonBox->button(QDialogButtonBox::Ok)->setEnabled(false);


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
    ui->numeroEdit->setText( QString::number( tili.numero()));
    ui->tasoSpin->setValue( tili.otsikkotaso());

    ui->tyyppiCombo->setCurrentIndex( ui->tyyppiCombo->findData( tili.tyyppi()) );
    ui->vastatiliEdit->valitseTiliNumerolla(tili.json()->luku("Vastatili") );

    ui->veroSpin->setValue( tili.json()->luku("AlvProsentti"));

    int alvlaji = tili.json()->luku("AlvLaji");

    ui->veroCombo->setCurrentIndex( ui->veroCombo->findData( alvlaji , VerotyyppiModel::KoodiRooli) );

    tarkasta();
}

void TilinMuokkausDialog::veroEnablePaivita()
{
    // Jos veroton, niin eipä silloin laiteta alv-prosenttia
    ui->veroSpin->setEnabled( ui->veroCombo->currentData(VerotyyppiModel::KoodiRooli).toInt() != 0 );

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

void TilinMuokkausDialog::nroMuuttaaTyyppia(const QString &nroteksti)
{
    if( !nroteksti.isEmpty())
    {
        int ekanro = nroteksti.left(1).toInt();

        // Jos numero alkaa 1, pitää olla vastaavaa-tili
        // 2 pitää olla vastattavaa
        // 3-> pitää olla tulostili

        if( ekanro == 1 && !ui->tyyppiCombo->currentData().toString().startsWith('A'))
            ui->tyyppiCombo->setCurrentIndex( ui->tyyppiCombo->findData("A") );
        else if( ekanro == 2 && !ui->tyyppiCombo->currentData().toString().startsWith('B'))
            ui->tyyppiCombo->setCurrentIndex( ui->tyyppiCombo->findData("B") );
        else if( ekanro == 3 && ( ui->tyyppiCombo->currentData().toString().startsWith('A') ||
                             ui->tyyppiCombo->currentData().toString().startsWith('B')))
            ui->tyyppiCombo->setCurrentIndex( ui->tyyppiCombo->findData("C") );
        else if( ekanro > 3 && ( ui->tyyppiCombo->currentData().toString().startsWith('A') ||
                             ui->tyyppiCombo->currentData().toString().startsWith('B')))
            ui->tyyppiCombo->setCurrentIndex( ui->tyyppiCombo->findData("D") );
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

}

void TilinMuokkausDialog::accept()
{
    ui->buttonBox->setFocus();

    if( ui->tiliRadio->isChecked() && ui->tyyppiCombo->currentIndex() > -1)
    {

        int ekanumero = ui->numeroEdit->text().left(1).toInt();
        QChar tyyppikirjain = ui->tyyppiCombo->currentData().toString().at(0);

        // Tarkistetaan ensin, että tilinumero osuu oikeaan väliin
        if(  (tyyppikirjain == 'A' && ekanumero != 1) ||
              (tyyppikirjain == 'B' && ekanumero != 2) ||
              (tyyppikirjain == 'C' && ekanumero < 3 ) ||
              (tyyppikirjain == 'D' && ekanumero < 3)   )
        {
            QMessageBox::critical(this, tr("Tilinumero on virheellinen"),
                                  tr("<b>Tilinumero ei sovi tilin tyyppiin</b><br>"
                                     "Vastaavaa-tilit alkavat 1<br>"
                                     "Vastattavaa-tilit alkavat 2<br>"
                                     "Muut tilit alkavat 3..9"));
            return;
        }

    }
    // Kaikki kunnossa eli voidaan tallentaa modeliin
    QString tyyppi = ui->tyyppiCombo->currentData().toString();
    int taso = ui->tasoSpin->value();

    if( ui->otsikkoRadio->isChecked())
        tyyppi = QString("H%1").arg(ui->tasoSpin->value());
    else
        taso = 0;

    JsonKentta *json;
    Tili uusitili;
    if( !index_.isValid())
    {
        // Uusi tili
        uusitili.asetaNumero(ui->numeroEdit->text().toInt());
        uusitili.asetaNimi( ui->nimiEdit->text());
        uusitili.asetaTyyppi( tyyppi );
        uusitili.asetaOtsikkotaso( taso );

        json = uusitili.json();

    }
    else
    {
        // Päivitetään tili modeliin
        model_->setData(index_, ui->numeroEdit->text().toInt(), TiliModel::NroRooli);
        model_->setData(index_, ui->nimiEdit->text(), TiliModel::NimiRooli);
        model_->setData(index_, taso, TiliModel::OtsikkotasoRooli);
        model_->setData(index_, tyyppi, TiliModel::TyyppiRooli);
        json = model_->jsonIndeksilla( index_.row());
    }

    if( !taso )
    {
        // Tilistä kirjoitetaan json-kentät

        if( ui->vastatiliEdit->valittuTilinumero() )
            json->set("Vastatili", ui->vastatiliEdit->valittuTilinumero());
        else
            json->unset("Vastatili");

        json->set("AlvLaji", ui->veroCombo->currentData(VerotyyppiModel::KoodiRooli).toInt());
        json->set("AlvProsentti", ui->veroSpin->value());
    }

    if( uusitili.numero() )     // Lisätään uusi tili
        model_->lisaaTili( uusitili );


    QDialog::accept();
}

