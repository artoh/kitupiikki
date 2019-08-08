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
#include "model/laskutaulumodel.h"

#include <QSortFilterProxyModel>

TilioteKirjaaja::TilioteKirjaaja(QWidget *parent, TilioteModel::Tilioterivi rivi) :
    QDialog(parent),    
    ui(new Ui::TilioteKirjaaja),
    rivi_(rivi),
    kohdennusProxy_(new KohdennusProxyModel(this, rivi.pvm, rivi.kohdennus) ),
    maksuProxy_(new QSortFilterProxyModel(this)),
    laskut_( new LaskuTauluModel(this))
{
    ui->setupUi(this);

    ui->ylaTab->addTab(QIcon(":/pic/lisaa.png"), tr("Tilille"));
    ui->ylaTab->addTab(QIcon(":/pic/poista.png"), tr("Tililtä"));

    ui->alaTabs->addTab(QIcon(":/pic/lasku.png"), tr("Laskun maksu"));
    ui->alaTabs->addTab(QIcon(":/pic/lisaa.png"), tr("Tulo"));
    ui->alaTabs->addTab(QIcon(":/pic/siirra.png"), tr("Siirto"));

    ui->kohdennusCombo->setModel(kohdennusProxy_);
    ui->kohdennusCombo->setModelColumn(KohdennusModel::NIMI);
    ui->kohdennusCombo->setCurrentIndex( ui->kohdennusCombo->findData( rivi.kohdennus, KohdennusModel::IdRooli) );

    alaTabMuuttui(0);

    connect( ui->euroEdit, &KpEuroEdit::textChanged, this, &TilioteKirjaaja::euroMuuttuu);
    connect( ui->alaTabs, &QTabBar::currentChanged, this, &TilioteKirjaaja::alaTabMuuttui);
    connect( ui->ylaTab, &QTabBar::currentChanged, this, &TilioteKirjaaja::ylaTabMuuttui);

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
    }

    maksuProxy_->setSourceModel(laskut_);

    ui->maksuView->setModel(maksuProxy_);
    connect( ui->maksuView->selectionModel(), &QItemSelectionModel::currentRowChanged , this, &TilioteKirjaaja::valitseLasku);
    connect( ui->suodatusEdit, &QLineEdit::textEdited, this, &TilioteKirjaaja::suodata);

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


    if( ui->alaTabs->currentIndex() == MAKSU) {
        QModelIndex index = ui->maksuView->currentIndex();

        rivi_.saajamaksaja = index.data(LaskuTauluModel::AsiakasToimittajaNimiRooli).toString();
        rivi_.saajamaksajaId = index.data(LaskuTauluModel::AsiakasToimittajaIdRooli).toInt();
        rivi_.eraId = index.data(LaskuTauluModel::EraIdRooli).toInt();
        rivi_.laskupvm = index.data(LaskuTauluModel::LaskuPvmRooli).toDate();
        rivi_.tili = index.data(LaskuTauluModel::TiliRooli).toInt();
        if( rivi_.selite.isEmpty())
            rivi_.selite = index.data(LaskuTauluModel::OtsikkoRooli).toString();

    } else if( ui->alaTabs->currentIndex() == TULOMENO ) {
        rivi_.saajamaksajaId = ui->asiakastoimittaja->id();
        rivi_.saajamaksaja = ui->asiakastoimittaja->nimi();

        if( rivi_.selite.isEmpty())
            rivi_.selite = rivi_.saajamaksaja;
    }

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

    ui->asiakasLabel->setVisible( tab == TULOMENO);
    ui->asiakastoimittaja->setVisible( tab == TULOMENO );

    ui->seliteLabel->setVisible(tab != MAKSU);
    ui->seliteEdit->setVisible( tab != MAKSU);

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

        laskut_->lataaAvoimet( menoa_ );

    } else if( tab == TULOMENO ) {
        ui->tiliLabel->setText( menoa_ ? tr("Menotili") : tr("Tulotili"));
        ui->asiakasLabel->setText( menoa_ ? tr("Toimittaja") : tr("Asiakas"));
        ui->tiliEdit->suodataTyypilla( menoa_ ? "D.*" : "C.*");
        ui->asiakastoimittaja->alusta(menoa_);

    } else if ( tab == SIIRTO ) {
        ui->tiliLabel->setText( menoa_ ? tr("Tilille") : tr("Tililtä")  );
        ui->tiliEdit->suodataTyypilla( "[AB].*");
    }
}

void TilioteKirjaaja::euroMuuttuu()
{
   ui->ylaTab->setCurrentIndex( ui->euroEdit->miinus() ? 1 : 0 );
}

void TilioteKirjaaja::ylaTabMuuttui(int tab)
{
    menoa_ = tab;
    if( menoa_ ) {
        ui->alaTabs->setTabText(MAKSU, tr("Maksettu lasku"));
        ui->alaTabs->setTabIcon(TULOMENO, QIcon(":/pic/poista.png") ) ;
        ui->alaTabs->setTabText(TULOMENO, tr("Meno"));
    } else {
        ui->alaTabs->setTabText(MAKSU, tr("Saapuva maksu"));
        ui->alaTabs->setTabIcon(TULOMENO, QIcon(":/pic/lisaa.png") ) ;
        ui->alaTabs->setTabText(TULOMENO, tr("Tulo"));
    }
    alaTabMuuttui( ui->alaTabs->currentIndex() );


    ui->euroEdit->setMiinus( tab );
}

void TilioteKirjaaja::valitseLasku()
{
    QModelIndex index = ui->maksuView->currentIndex();

    if( index.isValid()) {
        double avoinna = index.data(LaskuTauluModel::AvoinnaRooli).toDouble();
        ui->euroEdit->setValue( menoa_ ? 0 - avoinna : avoinna  );
    }
}

void TilioteKirjaaja::suodata(const QString &teksti)
{
    if( teksti.startsWith("RF") || teksti.contains(  QRegularExpression("^\\d+$")))
    {
        maksuProxy_->setFilterKeyColumn( LaskuTauluModel::NUMERO );
        maksuProxy_->setFilterRegularExpression(QRegularExpression("^" + teksti));
    } else {
        maksuProxy_->setFilterKeyColumn( LaskuTauluModel::ASIAKASTOIMITTAJA);
        maksuProxy_->setFilterCaseSensitivity(Qt::CaseInsensitive);
        maksuProxy_->setFilterFixedString( teksti );
    }


}
