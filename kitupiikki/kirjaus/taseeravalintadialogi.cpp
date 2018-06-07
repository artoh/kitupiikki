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

#include "taseeravalintadialogi.h"
#include "ui_taseeravalintadialogi.h"

#include "kohdennusproxymodel.h"

#include "db/kirjanpito.h"
#include "db/tili.h"

#include <QDebug>
#include <QRegularExpressionValidator>

TaseEraValintaDialogi::TaseEraValintaDialogi(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::TaseEraValintaDialogi)
{
    ui->setupUi(this);

    sntProxy_ = new QSortFilterProxyModel(this);
    sntProxy_->setSourceModel(&model_);
    sntProxy_->setFilterRole(EranValintaModel::SaldoRooli);

    proxy_ = new QSortFilterProxyModel(this);
    proxy_->setSortRole(EranValintaModel::PvmRooli);
    proxy_->setFilterCaseSensitivity(Qt::CaseInsensitive);
    proxy_->setSourceModel(sntProxy_);

    ui->view->setModel( proxy_);
    ui->view->setSelectionMode(QListView::SingleSelection);

    ui->summaEdit->setValidator(new QRegularExpressionValidator(QRegularExpression("[-]?\\d+(,\\d\\d)?"), this));

    connect( ui->suodatusEdit, SIGNAL(textChanged(QString)), proxy_, SLOT(setFilterFixedString(QString)));
    connect( ui->view->selectionModel(), SIGNAL(currentChanged(QModelIndex,QModelIndex)), this, SLOT(eraValintaVaihtuu()) );

    connect( ui->vainAvoimetCheck, SIGNAL(clicked(bool)), this, SLOT(sntSuodatusVaihtuu()));
    connect( ui->summaEdit, SIGNAL(textEdited(QString)), this, SLOT(sntSuodatusVaihtuu()));
}

TaseEraValintaDialogi::~TaseEraValintaDialogi()
{
    delete ui;
}




bool TaseEraValintaDialogi::nayta(VientiModel *model, QModelIndex &index)
{
    tili_ = kp()->tilit()->tiliNumerolla( index.data(VientiModel::TiliNumeroRooli).toInt() );
    taseEra_ = index.data( VientiModel::EraIdRooli).toInt();
    vientiId_ = index.data( VientiModel::IdRooli).toInt();

    model_.lataa( tili_, true );

    ui->view->setCurrentIndex( proxy_->index(0,0));

    ui->tiliEdit->setText( index.data(VientiModel::IbanRooli).toString());
    ui->viiteEdit->setText( index.data(VientiModel::ViiteRooli).toString());
    ui->nimiEdit->setText( index.data(VientiModel::AsiakasRooli).toString());
    QDate laskupvm = index.data(VientiModel::LaskuPvmRooli).toDate();

    if( laskupvm.isValid())
        ui->laskunpvmEdit->setDate( index.data(VientiModel::LaskuPvmRooli).toDate());
    else
        ui->laskunpvmEdit->setDate( index.data(VientiModel::PvmRooli).toDate() );

    QDate erapvm = index.data( VientiModel::EraPvmRooli ).toDate();

    ui->erapvmCheck->setChecked( erapvm.isValid());
    ui->eraDate->setEnabled( erapvm.isValid());

    if( !erapvm.isValid()  )
        erapvm = kp()->paivamaara();
    ui->eraDate->setDate( erapvm );

    if( tili_.onko(TiliLaji::TASAERAPOISTO))
    {
        int poistokk = index.data( VientiModel::PoistoKkRooli).toInt();
        if( !poistokk )
            poistokk = tili_.json()->luku("Tasaerapoisto");
        ui->poistoSpin->setValue( poistokk / 12 );
    }

    eraValintaVaihtuu();

    // Jos tehdään ostovelkaa, näytetään oletuksena ostotiedot
    if( (eraId()==TaseEra::UUSIERA || eraId() == index.data(VientiModel::IdRooli).toInt() ) && (tili_.onko(TiliLaji::OSTOVELKA) || tili_.onko(TiliLaji::MYYNTISAATAVA) ) )
        ui->tabWidget->setCurrentIndex( OSTO_TAB );

    ui->tiliEdit->setEnabled( tili_.onko(TiliLaji::OSTOVELKA) );

    if( tili_.onko(TiliLaji::OSTOVELKA))
        ui->nimiLabel->setText(tr("Myyjän nimi"));
    else if( tili_.onko(TiliLaji::MYYNTISAATAVA))
        ui->nimiLabel->setText(tr("Asiakkaan nimi"));


    bool kohdennuskaytossa = tili_.json()->luku("Kohdennukset");
    ui->kohdennusLabel->setVisible(kohdennuskaytossa);
    ui->kohdennusCombo->setVisible(kohdennuskaytossa);

    if( kohdennuskaytossa )
    {
        KohdennusProxyModel *kohdennusProxy = new KohdennusProxyModel(this, index.data(VientiModel::PvmRooli).toDate(), index.data(VientiModel::KohdennusRooli).toInt());
        ui->kohdennusCombo->setModel(kohdennusProxy);
        ui->kohdennusCombo->setCurrentIndex( ui->kohdennusCombo->findData( index.data(VientiModel::KohdennusRooli), KohdennusModel::IdRooli ) );
    }

    sntSuodatusVaihtuu();

    if( exec())
    {
        model->setData( index, eraId(), VientiModel::EraIdRooli);
        model->setData( index, poistoKk(), VientiModel::PoistoKkRooli);

        if( tili_.onko(TiliLaji::OSTOVELKA) || tili_.onko(TiliLaji::MYYNTISAATAVA) )
        {
            model->setData( index, ui->tiliEdit->text(), VientiModel::IbanRooli);
            model->setData( index, ui->viiteEdit->text(), VientiModel::ViiteRooli);

            if( ui->erapvmCheck->isChecked())
                model->setData( index, ui->eraDate->date(), VientiModel::EraPvmRooli);
            else
                model->setData( index, QDate() , VientiModel::EraPvmRooli);

            model->setData( index, ui->nimiEdit->text(), VientiModel::AsiakasRooli);
            model->setData( index, ui->laskunpvmEdit->date(), VientiModel::LaskuPvmRooli );
        }
        if( kohdennuskaytossa)
            model->setData( index, ui->kohdennusCombo->currentData(KohdennusModel::IdRooli), VientiModel::KohdennusRooli);


        return true;
    }
    return false;

}

int TaseEraValintaDialogi::eraId()
{
    return ui->view->currentIndex().data(EranValintaModel::EraIdRooli).toInt();
}

int TaseEraValintaDialogi::poistoKk()
{
    if( tili_.onko(TiliLaji::TASAERAPOISTO)   && eraId() == TaseEra::UUSIERA )
        return ui->poistoSpin->value() * 12;
    else
        return 0;
}

void TaseEraValintaDialogi::eraValintaVaihtuu()
{

    ui->poistoLabel->setVisible( (eraId()==TaseEra::UUSIERA || eraId() == vientiId_ ) && tili_.onko(TiliLaji::TASAERAPOISTO) );
    ui->poistoSpin->setVisible( (eraId()==TaseEra::UUSIERA || eraId() == vientiId_ ) && tili_.onko(TiliLaji::TASAERAPOISTO) );

    ui->tabWidget->setTabEnabled( OSTO_TAB , (eraId()==TaseEra::UUSIERA || eraId() == vientiId_ ) && (tili_.onko(TiliLaji::OSTOVELKA) || tili_.onko(TiliLaji::MYYNTISAATAVA) ) );

}

void TaseEraValintaDialogi::sntSuodatusVaihtuu()
{

    if( ui->summaEdit->hasAcceptableInput() )
    {
        int sentit = std::round(ui->summaEdit->text().replace(',','.').toDouble() * 100);
        sntProxy_->setFilterRegExp( QString("^[-]?%1$").arg(sentit) );
    }
    else if( ui->vainAvoimetCheck->isChecked())
    {
        sntProxy_->setFilterRegExp("^[-1-9].*");
    }
    else
        sntProxy_->setFilterFixedString("");


    // Valitaan valittuna ollut
    for(int i=0; i < proxy_->rowCount(); i++)
    {
        if( proxy_->data( proxy_->index(i,0), EranValintaModel::EraIdRooli ).toInt() == taseEra_ )
        {
            ui->view->setCurrentIndex( proxy_->index(i,0) );
        }
    }
}
