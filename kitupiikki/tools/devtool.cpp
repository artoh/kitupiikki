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

#include "devtool.h"
#include "ui_devtool.h"

#include "db/kirjanpito.h"
#include "uusikp/skripti.h"

DevTool::DevTool(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::DevTool)
{
    ui->setupUi(this);

    connect( ui->avainEdit, SIGNAL(textChanged(QString)), this, SLOT(haeAsetus(QString)));

    connect( ui->tallennaAsetusNappi, SIGNAL(clicked(bool)), this, SLOT(tallennaAsetus()));
    connect( ui->poistaNappi, SIGNAL(clicked(bool)), this, SLOT(poistaAsetus()));

    connect( ui->suoritaNappi, &QPushButton::clicked,
             [this] { Skripti::suorita( ui->skriptiEdit->toPlainText().split('\n') ); });

    haeAvaimet();
    ui->avainLista->setCurrentRow(0);
}

DevTool::~DevTool()
{
    delete ui;
}

void DevTool::haeAsetus(const QString &asetus)
{
    ui->arvoEdit->setPlainText( kp()->asetukset()->asetus(asetus) );
}

void DevTool::tallennaAsetus()
{
    QString avain = ui->avainEdit->text();
    QString arvo = ui->arvoEdit->toPlainText();

    // Tallennetaan asetus
    kp()->asetukset()->aseta(avain, arvo);

    // Jos asetusta ei vielä ollut, lisätään se listaan

    QList<QListWidgetItem*> items = ui->avainLista->findItems(avain, Qt::MatchExactly);
    if( !arvo.isEmpty() && !items.count() )
    {
        // Etsitään paikka aakkosista
        int indeksi = 0;
        for( indeksi = 0; indeksi < ui->avainLista->count(); indeksi++)
            if( ui->avainLista->item(indeksi)->text() > avain)
            {
                ui->avainLista->insertItem(indeksi, avain);
                ui->avainLista->setCurrentRow(indeksi);
                return;
            }
        ui->avainLista->addItem(avain);
        ui->avainLista->setCurrentRow( ui->avainLista->count() );
    }

}

void DevTool::poistaAsetus()
{
    QString avain = ui->avainEdit->text();

    kp()->asetukset()->poista(avain);

    // Jos asetus poistui, poistetaan
    QList<QListWidgetItem*> items = ui->avainLista->findItems(avain, Qt::MatchExactly);
    if( items.count())
    {
        delete items.first();
    }
}

void DevTool::haeAvaimet()
{
    ui->avainEdit->clear();
    ui->avainLista->addItems( kp()->asetukset()->avaimet() );
}

