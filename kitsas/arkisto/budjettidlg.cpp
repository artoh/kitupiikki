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
#include "budjettikohdennusproxy.h"
#include "kirjaus/eurodelegaatti.h"

#include <QMessageBox>
#include <QCloseEvent>

BudjettiDlg::BudjettiDlg(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::BudjettiDlg)
{
    ui->setupUi(this);

    model_ = new BudjettiModel(this);
    kohdennukset_ = new BudjettiKohdennusProxy(this);

    ui->tilikausiCombo->setModel( kp()->tilikaudet() );
    ui->kohdennusCombo->setModel( kohdennukset_);
    ui->view->setModel( model_ );

    ui->view->setItemDelegateForRow( BudjettiModel::EUROT, new EuroDelegaatti(this) );

    connect( ui->tallennaNappi, &QPushButton::clicked, model_, &BudjettiModel::tallenna );
    connect( ui->tilikausiCombo, &QComboBox::currentTextChanged, this, &BudjettiDlg::kausivaihtuu);
    connect( ui->kohdennusCombo, &QComboBox::currentTextChanged, this, &BudjettiDlg::kohdennusVaihtuu);
    connect( model_, &BudjettiModel::summaMuuttui, this, &BudjettiDlg::muokattu);
    connect( ui->kopioiNappi, &QPushButton::clicked, model_, &BudjettiModel::kopioiEdellinen);
    connect( ui->peruNappi, &QPushButton::clicked, this, &BudjettiDlg::close );
    connect( ui->ohjeNappi, &QPushButton::clicked, [] { kp()->ohje("tilikaudet/budjetti"); });

    ui->view->setItemDelegateForColumn(BudjettiModel::EDELLINEN, new EuroDelegaatti);
    ui->view->setItemDelegateForColumn(BudjettiModel::EUROT, new EuroDelegaatti);

    ui->view->horizontalHeader()->setSectionResizeMode(BudjettiModel::NIMI, QHeaderView::Stretch );
}

BudjettiDlg::~BudjettiDlg()
{
    delete ui;
}

void BudjettiDlg::lataa(const QDate& kausi)
{
    ui->tilikausiCombo->setCurrentIndex( ui->tilikausiCombo->findData( kausi, TilikausiModel::AlkaaRooli ));
    ui->kohdennusCombo->setCurrentIndex( ui->kohdennusCombo->findData(0, KohdennusModel::IdRooli) );
    model_->lataa( kausi );
}

void BudjettiDlg::kausivaihtuu()
{
    kohdennukset_->asetaKausi( ui->tilikausiCombo->currentData(TilikausiModel::AlkaaRooli).toDate(),
                               ui->tilikausiCombo->currentData(TilikausiModel::PaattyyRooli).toDate());
    kysyTallennus();
    model_->lataa( ui->tilikausiCombo->currentData(TilikausiModel::AlkaaRooli).toDate() );
}

void BudjettiDlg::kohdennusVaihtuu()
{
    model_->nayta( ui->kohdennusCombo->currentData(KohdennusModel::IdRooli).toInt() );
}

void BudjettiDlg::muokattu(qlonglong summa, qlonglong kokosumma)
{
    if( summa == kokosumma)
        ui->summaLabel->setText( QString("Budjetoitu tulos %L1 €").arg( summa / 100.0, 10,'f',2));
    else
        ui->summaLabel->setText( QString("Budjetoitu tulos %L1 € ( Yhteensä %L2 € )").arg( summa / 100.0, 10,'f',2)
                                 .arg(kokosumma/100.0, 0, 'f', 2));

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


