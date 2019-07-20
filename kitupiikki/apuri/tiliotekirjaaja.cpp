/*
   Copyright (C) 2019 Arto Hyvättinen

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
#include "tiliotekirjaaja.h"
#include "ui_tiliotekirjaaja.h"

TilioteKirjaaja::TilioteKirjaaja(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::TilioteKirjaaja)
{
    ui->setupUi(this);

    ui->alaTabs->addTab(QIcon(":/pic/lasku.png"), tr("Laskun maksu"));
    ui->alaTabs->addTab(QIcon(":/pic/lisaa.png"), tr("Tulo"));
    ui->alaTabs->addTab(QIcon(":/pic/siirra.png"), tr("Siirto"));

    alaTabMuuttui(0);

    connect( ui->euroEdit, &KpEuroEdit::textChanged, this, &TilioteKirjaaja::euroMuuttuu);
    connect( ui->alaTabs, &QTabBar::currentChanged, this, &TilioteKirjaaja::alaTabMuuttui);

    ui->alaTabs->setVisible(false);
    alaTabMuuttui(PIILOSSA);
}

TilioteKirjaaja::~TilioteKirjaaja()
{
    delete ui;
}

void TilioteKirjaaja::asetaPvm(const QDate &pvm)
{
    ui->pvmEdit->setDate(pvm);
}


void TilioteKirjaaja::alaTabMuuttui(int tab)
{

    ui->suodatusEdit->setVisible( tab == MAKSU  );
    ui->maksuView->setVisible( tab == MAKSU );

    ui->tiliLabel->setVisible( tab != MAKSU && tab != PIILOSSA);
    ui->tiliEdit->setVisible( tab != MAKSU && tab != PIILOSSA);

    ui->kohdennusLabel->setVisible( tab == TULOMENO && kp()->kohdennukset()->kohdennuksia() );
    ui->kohdennusCombo->setVisible( tab == TULOMENO&& kp()->kohdennukset()->kohdennuksia() );

    ui->merkkausLabel->setVisible(  tab == TULOMENO && kp()->kohdennukset()->merkkauksia() );
    ui->merkkausCC->setVisible(  tab == TULOMENO && kp()->kohdennukset()->merkkauksia() );

    ui->asiakasLabel->setVisible( tab == TULOMENO);
    ui->asiakasEdit->setVisible(tab == TULOMENO);
    ui->asiakasBtn->setVisible( tab == TULOMENO);

    if( tab == MAKSU ) {

    } else if( tab == TULOMENO ) {
        ui->tiliLabel->setText( menoa_ ? tr("Menotili") : tr("Tulotili"));
        ui->asiakasLabel->setText( menoa_ ? tr("Saaja") : tr("Maksaja"));
        ui->tiliEdit->suodataTyypilla( menoa_ ? "D.*" : "C.*");

    } else if ( tab == SIIRTO ) {
        ui->tiliLabel->setText( menoa_ ? tr("Tilille") : tr("Tililtä")  );
        ui->tiliEdit->suodataTyypilla( "[AB].*");
    }
}

void TilioteKirjaaja::euroMuuttuu()
{
    qlonglong euro = ui->euroEdit->asCents();
    if( euro && !ui->alaTabs->isVisible()) {
        ui->ohjeLabel->hide();
        ui->alaTabs->setVisible(true);
        alaTabMuuttui( ui->alaTabs->currentIndex() );
    }


    bool menoa = euro < 0;

    if( menoa != menoa_) {
        menoa_ = menoa;
        if( menoa ) {
            ui->alaTabs->setTabText(MAKSU, tr("Maksettu lasku"));
            ui->alaTabs->setTabIcon(TULOMENO, QIcon(":/pic/poista.png") ) ;
            ui->alaTabs->setTabText(TULOMENO, tr("Meno"));
        } else {
            ui->alaTabs->setTabText(MAKSU, tr("Saapuva maksu"));
            ui->alaTabs->setTabIcon(TULOMENO, QIcon(":/pic/lisaa.png") ) ;
            ui->alaTabs->setTabText(TULOMENO, tr("Tulo"));
        }
        alaTabMuuttui( ui->alaTabs->currentIndex() );
    }

}
