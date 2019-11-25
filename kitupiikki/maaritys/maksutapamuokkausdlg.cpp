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
#include "maksutapamuokkausdlg.h"
#include "ui_maksutapamuokkausdlg.h"

#include "model/maksutapamodel.h"
#include "db/kirjanpito.h"
#include <QListWidgetItem>
#include <QDirIterator>

MaksutapaMuokkausDlg::MaksutapaMuokkausDlg(QWidget *parent) :
    QDialog(parent),    
    ui(new Ui::MaksutapaMuokkausDlg)
{
    ui->setupUi(this);
}

MaksutapaMuokkausDlg::~MaksutapaMuokkausDlg()
{
    delete ui;
}

QVariantMap MaksutapaMuokkausDlg::muokkaa(const QVariantMap &ladattu)
{
    for(QString kieli : kp()->asetukset()->kielet()) {
        QListWidgetItem* item = new QListWidgetItem( lippu(kieli), ladattu.value(kieli).toString() , ui->nimiLista  );
        item->setData(Qt::UserRole, kieli);
        item->setFlags( Qt::ItemIsEnabled | Qt::ItemIsEditable);
    }

    ui->tiliCombo->suodataTyypilla("[AB]");
    if( ladattu.value("TILI").toInt())
    ui->tiliCombo->valitseTili( ladattu.value("TILI").toInt() );
    ui->eraCheck->setChecked( ladattu.value("ERA").toInt() < 0);

    QString kuva = ladattu.value("KUVA").toString();

    QDirIterator iter(":/maksutavat");
    while( iter.hasNext() ) {
        QString polku = iter.next();
        QString nimi = polku.mid(polku.lastIndexOf("/")+1);
        nimi = nimi.left( nimi.indexOf("."));
        QListWidgetItem *item = new QListWidgetItem( QIcon(polku), QString(), ui->kuvakeLista  );
        item->setData(Qt::UserRole, nimi);
        item->setFlags(Qt::ItemIsEnabled | Qt::ItemIsSelectable);
        if( nimi == kuva)
            item->setSelected(true);
    }

    if( exec() == QDialog::Accepted ) {
        QVariantMap paluu;
        for(int i=0; i < ui->nimiLista->count(); i++) {
            QListWidgetItem *item = ui->nimiLista->item(i);
            if( !item->data(Qt::EditRole).toString().isEmpty())
                paluu.insert( item->data(Qt::UserRole).toString(),
                              item->data(Qt::DisplayRole));
        }
        paluu.insert("TILI", ui->tiliCombo->valittuTilinumero());
        if( ui->eraCheck->isChecked())
            paluu.insert("ERA", -1);
        if( ui->kuvakeLista->selectedItems().count() )
            paluu.insert("KUVA", ui->kuvakeLista->selectedItems().first()->data(Qt::UserRole));
        return paluu;
    }
    return QVariantMap();
}

