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

#include <QDialogButtonBox>
#include <QPushButton>

#include <QDebug>

#include "tositelajidialogi.h"
#include "ui_tositelajidialogi.h"

TositelajiDialogi::TositelajiDialogi(TositelajiModel *model, const QModelIndex &index, QWidget *parent) :
    QDialog(parent),
    ui(new Ui::TositelajiDialogi),
    model_(model),
    indeksi_(index)
{
    ui->setupUi(this);

    if( index.isValid())
        lataa();
    else
        ui->buttonBox->button(QDialogButtonBox::Ok)->setEnabled(false);

    ui->oletusTiliEdit->suodataTyypilla("[CD].*");
    ui->vastatiliEdit->suodataTyypilla("[AB].*");

    connect( ui->tunnusEdit, SIGNAL(textEdited(QString)), this, SLOT(tarkasta()));
    connect( ui->nimiEdit, SIGNAL(textEdited(QString)), this, SLOT(tarkasta()));
    connect( ui->vastatiliEdit, SIGNAL(textChanged(QString)), this, SLOT(vastatilivalittu()));


}

TositelajiDialogi::~TositelajiDialogi()
{
    delete ui;
}

void TositelajiDialogi::lataa()
{
    ui->tunnusEdit->setText( indeksi_.data( TositelajiModel::TunnusRooli).toString());
    ui->nimiEdit->setText( indeksi_.data(TositelajiModel::NimiRooli).toString());
    ui->vastatiliEdit->valitseTiliNumerolla( indeksi_.data(TositelajiModel::VastatiliNroRooli).toInt() );

    int kirjaustyyppi = indeksi_.data( TositelajiModel::KirjausTyyppiRooli).toInt();
    ui->kaikkiRadio->setChecked( kirjaustyyppi == TositelajiModel::KAIKKIKIRJAUKSET);
    ui->ostoRadio->setChecked( kirjaustyyppi == TositelajiModel::OSTOLASKUT);
    ui->myyntiRadio->setChecked( kirjaustyyppi == TositelajiModel::MYYNTILASKUT);
    ui->tilioteRadio->setChecked( kirjaustyyppi == TositelajiModel::TILIOTE);


    if( indeksi_.data( TositelajiModel::IdRooli).toInt() == 1)
    {
        // Oletustositelaji Muut lyhenne "" pitää säilyttää, ei saa muuttaa!
        ui->tunnusEdit->setEnabled(false);
    }

    vastatilivalittu();     // Rahatilille on mahdollista valita pankkitili
}

void TositelajiDialogi::tarkasta()
{

    // Tunnisteen pitää olla uniikki
    if( !indeksi_.isValid()  ||
        ui->tunnusEdit->text() != indeksi_.data(TositelajiModel::TunnusRooli).toString())
    {
        for( int i=0; i < model_->rowCount( QModelIndex() ); i++ )
        {
            if( model_->data( model_->index(i,0) , TositelajiModel::TunnusRooli).toString() == ui->tunnusEdit->text() )
            {
                ui->buttonBox->button(QDialogButtonBox::Ok)->setEnabled(false);
                ui->tunnusEdit->setStyleSheet("color: red;");
                return;
            }
        }
    }
    // Ei löytynyt toista samaa
    ui->tunnusEdit->setStyleSheet("color: black;");

    // Nyt vielä tarkastetaan että onko tekstiä paikallaan
    ui->buttonBox->button(QDialogButtonBox::Ok)->setEnabled( !ui->nimiEdit->text().isEmpty() );

}

void TositelajiDialogi::vastatilivalittu()
{
    Tili tili = kp()->tilit()->tiliNumerolla( ui->vastatiliEdit->valittuTilinumero() );

    ui->tilioteRadio->setEnabled( tili.onkoRahaTili() );

    if( tili.onkoRahaTili())
        ui->tilioteRadio->setText( tr("Tiliotteita tilille %1 %2").arg(tili.numero()).arg(tili.nimi()));
}

void TositelajiDialogi::accept()
{
    int kirjaustyyppi = TositelajiModel::KAIKKIKIRJAUKSET;
    if( ui->ostoRadio->isChecked())
        kirjaustyyppi = TositelajiModel::OSTOLASKUT;
    else if( ui->myyntiRadio->isChecked())
        kirjaustyyppi = TositelajiModel::MYYNTILASKUT;
    else if( ui->tilioteRadio->isChecked())
        kirjaustyyppi = TositelajiModel::TILIOTE;


    if( !indeksi_.isValid())
        indeksi_ = model_->lisaaRivi();


    model_->setData( indeksi_.sibling( indeksi_.row(), TositelajiModel::TUNNUS ) , ui->tunnusEdit->text(), Qt::EditRole );
    model_->setData( indeksi_.sibling( indeksi_.row(), TositelajiModel::NIMI ) , ui->nimiEdit->text(), Qt::EditRole );
    model_->setData( indeksi_.sibling( indeksi_.row(), TositelajiModel::VASTATILI ) , ui->vastatiliEdit->valittuTilinumero() , Qt::EditRole );
    model_->setData( indeksi_, kirjaustyyppi , TositelajiModel::KirjausTyyppiRooli );
    model_->setData( indeksi_, ui->oletusTiliEdit->valittuTilinumero(), TositelajiModel::OletustiliRooli );

    QDialog::accept();
}
