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

#include "kohdennusdialog.h"
#include "ui_kohdennusdialog.h"

#include "db/kirjanpito.h"

#include <QListWidgetItem>

#include <QSortFilterProxyModel>

KohdennusDialog::KohdennusDialog(int index, QWidget *parent)
    : QDialog(parent),
     ui(new Ui::KohdennusDialog),
     index_(index)

{
    ui->setupUi(this);

    // Oletuskestot
    Tilikausi nykyinen = kp()->tilikausiPaivalle( kp()->paivamaara() );
    ui->alkaaDate->setDate( nykyinen.alkaa());
    ui->paattyyDate->setDate( nykyinen.paattyy());

    ui->alkaaDate->setMinimumDate( kp()->tilikaudet()->kirjanpitoAlkaa() );
    ui->paattyyDate->setMinimumDate( kp()->tilikaudet()->kirjanpitoAlkaa() );

    connect( ui->alkaaDate, SIGNAL(dateChanged(QDate)), this, SLOT(tarkennaLoppuMinimi()));

    if( index_ > -1)
        lataa();
    else {
        for(QString kieli : kp()->asetukset()->kielet() ) {
            QListWidgetItem* item = new QListWidgetItem( lippu(kieli), "" , ui->nimiList);
            item->setData(Qt::UserRole, kieli);
            item->setFlags( Qt::ItemIsEnabled | Qt::ItemIsEditable);
        }

    }

    QSortFilterProxyModel *proxy = new QSortFilterProxyModel(this);
    proxy->setSourceModel(kp()->kohdennukset());
    proxy->setFilterRole(KohdennusModel::TyyppiRooli);
    proxy->setFilterRegExp("[01]");
    proxy->setSortRole(KohdennusModel::NimiRooli);
    ui->kustannuspaikkaCombo->setModel(proxy);

    tyyppiMuuttuu();
    connect( ui->kustannuspaikkaRadio, &QRadioButton::clicked, this, &KohdennusDialog::tyyppiMuuttuu );
    connect( ui->projektiRadio, &QRadioButton::clicked, this, &KohdennusDialog::tyyppiMuuttuu );
    connect( ui->tagRadio, &QRadioButton::clicked, this, &KohdennusDialog::tyyppiMuuttuu );
    connect( ui->buttonBox, &QDialogButtonBox::helpRequested, [] { kp()->ohje("maaritykset/kohdennukset");});
}

KohdennusDialog::~KohdennusDialog()
{
    delete ui;
}

void KohdennusDialog::tarkennaLoppuMinimi()
{
    if( ui->alkaaDate->date().daysTo( ui->paattyyDate->minimumDate() ) < 0)
    {
        // Päättyy ennen kuin alkaakaan, sehän ei passaa!
        ui->paattyyDate->setMinimumDate( ui->alkaaDate->date());
    }
}

void KohdennusDialog::tyyppiMuuttuu()
{
    ui->kustannuspaikkaLabel->setVisible( ui->projektiRadio->isChecked() );
    ui->kustannuspaikkaCombo->setVisible( ui->projektiRadio->isChecked() );
}

void KohdennusDialog::accept()
{
    {
        tallenna();
        QDialog::accept();
    }
}

void KohdennusDialog::lataa()
{
    Kohdennus* kohdennus = kp()->kohdennukset()->pkohdennus(index_);

    ui->kustannuspaikkaRadio->setChecked( kohdennus->tyyppi() == Kohdennus::KUSTANNUSPAIKKA);
    ui->projektiRadio->setChecked(kohdennus->tyyppi() == Kohdennus::PROJEKTI );
    ui->tagRadio->setChecked(kohdennus->tyyppi() == Kohdennus::MERKKAUS );

    for(QString kieli : kp()->asetukset()->kielet() ) {
        QListWidgetItem* item = new QListWidgetItem( lippu(kieli), kohdennus->kaannos(kieli) , ui->nimiList );
        item->setData(Qt::UserRole, kieli);
        item->setFlags( Qt::ItemIsEnabled | Qt::ItemIsEditable);
    }

    ui->maaraaikainenCheck->setChecked( kohdennus->alkaa().isValid() );
    ui->alkaaDate->setDate( kohdennus->alkaa());
    ui->paattyyDate->setDate( kohdennus->paattyy());

    if( ui->projektiRadio->isChecked())
        ui->kustannuspaikkaCombo->setCurrentIndex( ui->kustannuspaikkaCombo->findData( kohdennus->kuuluu(), KohdennusModel::IdRooli ) );
}

void KohdennusDialog::tallenna()
{
    Kohdennus::KohdennusTyyppi tyyppi = Kohdennus::EIKOHDENNETA;
    if( ui->kustannuspaikkaRadio->isChecked())
        tyyppi = Kohdennus::KUSTANNUSPAIKKA;
    else if( ui->projektiRadio->isChecked())
        tyyppi = Kohdennus::PROJEKTI;
    else if( ui->tagRadio->isChecked())
        tyyppi = Kohdennus::MERKKAUS;

    Kohdennus* kohdennus = index_ > 0 ? kp()->kohdennukset()->pkohdennus(index_) : kp()->kohdennukset()->lisaa( tyyppi );

    for(int i=0; i < ui->nimiList->count(); i++)
        kohdennus->asetaNimi( ui->nimiList->item(i)->text(), ui->nimiList->item(i)->data(Qt::UserRole).toString() );
    kohdennus->asetaTyyppi(tyyppi);
    kohdennus->asetaAlkaa( ui->maaraaikainenCheck->isChecked() ? ui->alkaaDate->date() : QDate());
    kohdennus->asetaPaattyy( ui->maaraaikainenCheck->isChecked() ? ui->paattyyDate->date() : QDate());
    if( tyyppi == Kohdennus::PROJEKTI)
        kohdennus->asetaKuuluu(ui->kustannuspaikkaCombo->currentData(KohdennusModel::IdRooli).toInt());

    kp()->kohdennukset()->tallenna( index_ > 0 ? index_ : kp()->kohdennukset()->rowCount()-1);

    QDialog::accept();
}


