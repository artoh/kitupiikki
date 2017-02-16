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

#include "tilinmuokkausdialog.h"
#include "db/tilimodel.h"
#include "db/tilinvalintaline.h"

TilinMuokkausDialog::TilinMuokkausDialog(TiliModel *model, QModelIndex index) :
    QDialog(), model_(model), index_(index)
{
    ui = new Ui::tilinmuokkausDialog();
    ui->setupUi(this);

    QMapIterator<QString,QString> iter( model_->tiliTyyppiTaulu() );

    // Laitetaan tyyppivaihtoehdot paikalleen
    while( iter.hasNext() )
    {
        iter.next();
        ui->tyyppiCombo->addItem( QIcon(),iter.value(), iter.key());
    }

    lataa();
}

TilinMuokkausDialog::~TilinMuokkausDialog()
{
    delete ui;
}

void TilinMuokkausDialog::lataa()
{
    Tili tili = model_->tiliIndeksilla( index_.row());

    ui->tiliRadio->setChecked( tili.otsikkotaso() == 0);
    ui->otsikkoRadio->setChecked( tili.otsikkotaso() );

    ui->nimiEdit->setText( tili.nimi());
    ui->numeroEdit->setText( QString::number( tili.numero()));
    ui->tasoSpin->setValue( tili.otsikkotaso());

    ui->tyyppiCombo->setCurrentIndex( ui->tyyppiCombo->findData( tili.tyyppi()) );

    ui->vastatiliEdit->valitseTiliIdlla( tili.json()->luku("Vastatili") );
    ui->veroSpin->setValue( tili.json()->luku("AlvProsentti"));


}
