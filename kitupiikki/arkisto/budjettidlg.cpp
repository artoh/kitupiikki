/*
   Copyright (C) 2018 Arto Hyvättinen

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
#include "budjettidlg.h"
#include "ui_budjettidlg.h"

#include "db/kirjanpito.h"
#include "budjettimodel.h"
#include "kirjaus/kohdennusproxymodel.h"

#include <QMessageBox>

BudjettiDlg::BudjettiDlg(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::BudjettiDlg)
{
    ui->setupUi(this);

    model_ = new BudjettiModel(this);
    kohdennukset_ = new KohdennusProxyModel(this);

    ui->tilikausiCombo->setModel( kp()->tilikaudet() );
    ui->kohdennusCombo->setModel( kohdennukset_);
    ui->view->setModel( model_ );

    connect( ui->tallennaNappi, &QPushButton::clicked, model_, &BudjettiModel::tallenna );
    connect( ui->tilikausiCombo, &QComboBox::currentTextChanged, this, &BudjettiDlg::kausivaihtuu);
    connect( ui->kohdennusCombo, &QComboBox::currentTextChanged, this, &BudjettiDlg::paivita);
    connect( model_, &BudjettiModel::summaMuuttui, this, &BudjettiDlg::muokattu);
    connect( ui->kopioiNappi, &QPushButton::clicked, model_, &BudjettiModel::kopioiEdellinen);
    connect( ui->peruNappi, &QPushButton::clicked, this, &BudjettiDlg::close );
    connect( ui->ohjeNappi, &QPushButton::clicked, [] { kp()->ohje("tilikaudet/budjetti"); });

    ui->view->horizontalHeader()->setSectionResizeMode(BudjettiModel::NIMI, QHeaderView::Stretch );
}

BudjettiDlg::~BudjettiDlg()
{
    delete ui;
}

void BudjettiDlg::lataa(const QString &kausi)
{
    ui->tilikausiCombo->setCurrentIndex( ui->tilikausiCombo->findData( kausi, TilikausiModel::LyhenneRooli ));
    ui->kohdennusCombo->setCurrentIndex( ui->kohdennusCombo->findData(0, KohdennusModel::IdRooli) );
    paivita();
}

void BudjettiDlg::kausivaihtuu()
{
    kohdennukset_->asetaPaiva( ui->tilikausiCombo->currentData(TilikausiModel::AlkaaRooli).toDate() );
    paivita();
}

void BudjettiDlg::paivita()
{
    kysyTallennus();
    model_->lataa( ui->tilikausiCombo->currentData(TilikausiModel::AlkaaRooli).toDate(),
                   ui->kohdennusCombo->currentData(KohdennusModel::IdRooli).toInt() );
}

void BudjettiDlg::muokattu(qlonglong summa)
{
    ui->summaLabel->setText( QString("Budjetoitu tulos %L1 €").arg( summa / 100.0, 10,'f',2));
    ui->tallennaNappi->setEnabled( model_->onkoMuokattu() );
    ui->kopioiNappi->setEnabled( ui->tilikausiCombo->currentIndex() > 0 );
}


void BudjettiDlg::kysyTallennus()
{
    if( model_->onkoMuokattu() )
    {
        if( QMessageBox::question(this, tr("Budjettia muokattu"),
                                  tr("Tallennetaanko muokattu budjetti?"),
                                  QMessageBox::Yes | QMessageBox::No,
                                  QMessageBox::Yes) == QMessageBox::Yes)
            model_->tallenna();
    }
}

void BudjettiDlg::closeEvent(QCloseEvent *event)
{
    kysyTallennus();
    event->accept();
}


