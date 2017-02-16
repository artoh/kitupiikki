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

#include "tilinmuokkausdialog.h"
#include "db/tilimodel.h"
#include "db/tilinvalintaline.h"

TilinMuokkausDialog::TilinMuokkausDialog(TiliModel *model, QModelIndex index) :
    QDialog(), model_(model), index_(index)
{
    ui = new Ui::tilinmuokkausDialog();
    ui->setupUi(this);

    ui->numeroEdit->setValidator( new QIntValidator(1,99999999, ui->numeroEdit));

    // Laitetaan tyyppivaihtoehdot paikalleen
    QMapIterator<QString,QString> iter( model_->tiliTyyppiTaulu() );
    while( iter.hasNext() )
    {
        iter.next();
        ui->tyyppiCombo->addItem( QIcon(),iter.value(), iter.key());
    }

    // Laitetaa verotyypit paikalleen
    QMapIterator<int,QString> veroIter(model->veroTyyppiTaulu());
    while( iter.hasNext())
    {
        iter.next();
        ui->veroCombo->addItem(QIcon(), iter.value(), iter.key());
    }


    connect( ui->veroCombo, SIGNAL(currentIndexChanged(int)), this, SLOT(veroEnablePaivita()));
    connect( ui->numeroEdit, SIGNAL(textChanged(QString)), this, SLOT(otsikkoTasoPaivita()));

    lataa();

}

TilinMuokkausDialog::~TilinMuokkausDialog()
{
    delete ui;
}

void TilinMuokkausDialog::lataa()
{
    Tili tili = model_->tiliIndeksilla( index_.row());

    ui->tasoSpin->setVisible(false);
    ui->tasoLabel->setVisible(false);

    ui->tiliRadio->setChecked( tili.otsikkotaso() == 0);
    ui->otsikkoRadio->setChecked( tili.otsikkotaso() );

    ui->nimiEdit->setText( tili.nimi());
    ui->numeroEdit->setText( QString::number( tili.numero()));
    ui->tasoSpin->setValue( tili.otsikkotaso());

    ui->tyyppiCombo->setCurrentIndex( ui->tyyppiCombo->findData( tili.tyyppi()) );

    ui->vastatiliEdit->valitseTiliIdlla( tili.json()->luku("Vastatili") );
    ui->veroSpin->setValue( tili.json()->luku("AlvProsentti"));
    ui->veroCombo->setCurrentIndex( ui->tyyppiCombo->findData( tili.json()->luku("AlvLaji")));
}

void TilinMuokkausDialog::veroEnablePaivita()
{
    // Jos veroton, niin eipä silloin laiteta alv-prosenttia
    ui->veroSpin->setEnabled( ui->veroCombo->currentData().toInt() != 0 );
}

void TilinMuokkausDialog::otsikkoTasoPaivita()
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
