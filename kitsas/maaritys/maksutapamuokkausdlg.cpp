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

#include "db/kirjanpito.h"
#include <QListWidgetItem>
#include <QDirIterator>

MaksutapaMuokkausDlg::MaksutapaMuokkausDlg(QWidget *parent) :
    QDialog(parent),    
    ui(new Ui::MaksutapaMuokkausDlg)
{
    ui->setupUi(this);
    connect( ui->buttonBox, &QDialogButtonBox::helpRequested, [] { kp()->ohje("asetukset/maksutavat");});
}

MaksutapaMuokkausDlg::~MaksutapaMuokkausDlg()
{
    delete ui;
}

Maksutapa MaksutapaMuokkausDlg::muokkaaMaksutapa(const Maksutapa& muokattava)
{
    ui->nimiLista->lataa(muokattava);

    ui->tiliCombo->suodataTyypilla("[AB]");
    ui->tiliCombo->valitseTili( muokattava.tili() );
    ui->eraCheck->setChecked( muokattava.uusiEra() );

    QString kuva = muokattava.kuva();

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
        Maksutapa muokattu = ui->nimiLista->tekstit().map();
        muokattu.asetaTili( ui->tiliCombo->valittuTilinumero() );
        muokattu.asetaUusiEra( ui->eraCheck->isChecked() );
        if( ui->kuvakeLista->selectedItems().count() )
            muokattu.asetaKuva(ui->kuvakeLista->selectedItems().first()->data(Qt::UserRole).toString());
        return muokattu;
    }
    return muokattava;
}

