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
#include "tositesarjamaaritys.h"
#include "db/kirjanpito.h"
#include "db/tositetyyppimodel.h"

#include <QJsonDocument>

TositesarjaMaaritys::TositesarjaMaaritys() :
    ui( new Ui::Tositesarjat)
{
    ui->setupUi(this);
}

bool TositesarjaMaaritys::nollaa()
{
    ui->eriSarjaan->setChecked( kp()->asetukset()->onko("erisarjaan") );
    ui->samaRadio->setChecked( !ui->eriSarjaan->isChecked());
    ui->kateisCheck->setChecked( kp()->asetukset()->onko("kateissarjaan") );

    QVariantMap map = QJsonDocument::fromJson( kp()->asetus("tositesarjat").toUtf8() ).toVariant().toMap();
    ui->kateisLine->setText( map.value("K").toString() );

    ui->view->setColumnCount(2);
    ui->view->setEnabled( ui->eriSarjaan->isChecked() );
    ui->kateisLine->setEnabled( ui->kateisCheck->isChecked());

    int rivi = 0;
    for(int i=0; i < kp()->tositeTyypit()->rowCount(); i++) {
        int koodi = kp()->tositeTyypit()->index(i,0).data(TositeTyyppiModel::KoodiRooli).toInt();
        if( koodi < TositeTyyppi::JARJESTELMATOSITE) {
            ui->view->setRowCount(rivi+1);
            QTableWidgetItem* item = new QTableWidgetItem( kp()->tositeTyypit()->kuvake(koodi),
                                                           kp()->tositeTyypit()->nimi(koodi));
            item->setFlags( Qt::ItemIsEnabled);
            ui->view->setItem(rivi,0, item);
            item = new QTableWidgetItem( map.value( QString::number(koodi) ).toString() );
            item->setData(Qt::UserRole, koodi);
            item->setFlags( Qt::ItemIsEnabled | Qt::ItemIsEditable);
            ui->view->setItem(rivi, 1, item);
            rivi++;
        }
    }
    ui->view->setRowCount(rivi+1);
    QTableWidgetItem *item = new QTableWidgetItem( QIcon(":/pic/ratas.png"), tr("Järjestelmätositteet") );
    item->setFlags( Qt::ItemIsEnabled);
    ui->view->setItem(rivi,0,item);
    item = new QTableWidgetItem( map.value("*").toString() );
    item->setFlags( Qt::ItemIsEnabled | Qt::ItemIsEditable);
    ui->view->setItem(rivi, 1, item);
    ui->view->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Stretch);

    connect( ui->eriSarjaan, &QRadioButton::clicked, this, &TositesarjaMaaritys::tallenna);
    connect( ui->samaRadio, &QRadioButton::clicked, this, &TositesarjaMaaritys::tallenna);
    connect( ui->kateisCheck, &QRadioButton::clicked, this, &TositesarjaMaaritys::tallenna);
    connect( ui->kateisLine, &QLineEdit::editingFinished, this, &TositesarjaMaaritys::tallenna);
    connect( ui->view, &QTableWidget::cellChanged , this, &TositesarjaMaaritys::tallenna);

    return true;
}

bool TositesarjaMaaritys::tallenna()
{
    QVariantMap asetukset;
    asetukset.insert("erisarjaan", ui->eriSarjaan->isChecked() ? "ON" : "EI");
    asetukset.insert("kateissarjaan", ui->kateisCheck->isChecked());

    if( ui->eriSarjaan->isChecked()) {

        QVariantMap sarjamap;
        sarjamap.insert("K", ui->kateisLine->text());
        for(int i=0; i < ui->view->rowCount(); i++) {
            sarjamap.insert( ui->view->item(i,1)->data(Qt::UserRole).toString(),
                             ui->view->item(i,1)->data(Qt::DisplayRole));
        }
        asetukset.insert("tositesarjat", QJsonDocument::fromVariant(sarjamap).toJson());
    }

    kp()->asetukset()->aseta(asetukset);
    return true;
}
