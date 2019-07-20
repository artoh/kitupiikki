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

#include "kirjaus/kohdennusproxymodel.h"

TilioteKirjaaja::TilioteKirjaaja(QWidget *parent, TilioteModel::Tilioterivi rivi) :
    QDialog(parent),    
    ui(new Ui::TilioteKirjaaja),
    rivi_(rivi),
    kohdennusProxy_(new KohdennusProxyModel(this, rivi.pvm, rivi.kohdennus) )
{
    ui->setupUi(this);

    ui->alaTabs->addTab(QIcon(":/pic/lasku.png"), tr("Laskun maksu"));
    ui->alaTabs->addTab(QIcon(":/pic/lisaa.png"), tr("Tulo"));
    ui->alaTabs->addTab(QIcon(":/pic/siirra.png"), tr("Siirto"));

    ui->kohdennusCombo->setModel(kohdennusProxy_);
    ui->kohdennusCombo->setModelColumn(KohdennusModel::NIMI);
    ui->kohdennusCombo->setCurrentIndex( ui->kohdennusCombo->findData( rivi.kohdennus, KohdennusModel::IdRooli) );

    alaTabMuuttui(0);

    connect( ui->euroEdit, &KpEuroEdit::textChanged, this, &TilioteKirjaaja::euroMuuttuu);
    connect( ui->alaTabs, &QTabBar::currentChanged, this, &TilioteKirjaaja::alaTabMuuttui);

    if( qAbs(rivi.euro) > 1e-5)
    {
        ui->euroEdit->setValue( rivi.euro );
        euroMuuttuu();
        Tili tili = kp()->tilit()->tiliNumerolla( rivi.tili );
        ui->tiliEdit->valitseTili(tili);

        if( rivi_.eraId )
            ui->alaTabs->setCurrentIndex(MAKSU);
        else if( tili.onko(TiliLaji::TULOS))
            ui->alaTabs->setCurrentIndex(TULOMENO);
        else
            ui->alaTabs->setCurrentIndex(SIIRTO);


        // Vielä erän valitseminen

    } else {
        ui->alaTabs->setVisible(false);
        alaTabMuuttui(PIILOSSA);
    }
}

TilioteKirjaaja::~TilioteKirjaaja()
{
    delete ui;
}

void TilioteKirjaaja::asetaPvm(const QDate &pvm)
{
    ui->pvmEdit->setDate(pvm);
}

TilioteModel::Tilioterivi TilioteKirjaaja::rivi()
{
    rivi_.pvm = ui->pvmEdit->date();
    rivi_.euro = ui->euroEdit->value();

    rivi_.selite = ui->seliteEdit->text();
    rivi_.tili = ui->tiliEdit->valittuTilinumero();
    rivi_.kohdennus = ui->kohdennusCombo->currentData(KohdennusModel::IdRooli).toInt();
    rivi_.merkkaukset.clear();
    for(auto var : ui->merkkausCC->selectedDatas())
        rivi_.merkkaukset.append( var.toInt() );


    return rivi_;
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

    if( tab == MAKSU ) {
        // Kohdennukset
        kohdennusProxy_->asetaPaiva(ui->pvmEdit->date());

        KohdennusProxyModel merkkausproxy(this, ui->pvmEdit->date(), -1, KohdennusProxyModel::MERKKKAUKSET );
        ui->merkkausCC->clear();

        for(int i=0; i < merkkausproxy.rowCount(); i++) {
            int koodi = merkkausproxy.data( merkkausproxy.index(i,0), KohdennusModel::IdRooli ).toInt();
            QString nimi = merkkausproxy.data( merkkausproxy.index(i,0), KohdennusModel::NimiRooli ).toString();

            Qt::CheckState state = rivi_.merkkaukset.contains( koodi ) ? Qt::Checked : Qt::Unchecked;
            ui->merkkausCC->addItem(nimi, koodi, state);
        }

    } else if( tab == TULOMENO ) {
        ui->tiliLabel->setText( menoa_ ? tr("Menotili") : tr("Tulotili"));
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
