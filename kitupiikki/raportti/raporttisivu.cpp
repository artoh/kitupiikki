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

#include <QListWidget>
#include <QFrame>
#include <QHBoxLayout>

#include <QFile>
#include <QTemporaryFile>
#include <QUrl>

#include <QDebug>

#include <QPrintDialog>
#include <QDesktopServices>

#include "raporttisivu.h"

#include "paivakirjaraportti.h"
#include "db/kirjanpito.h"

#include "raporittitesti.h"

RaporttiSivu::RaporttiSivu(QWidget *parent) : KitupiikkiSivu(parent),
    nykyraportti(0), printer(QPrinter::HighResolution)
{
    ui = new Ui::RaporttiWg();
    ui->setupUi(this);

    lisaaRaportti(new PaivakirjaRaportti);

    lisaaRaportti(new RaporttiTesti("Tuloslaskelma"));

    ui->lista->setCurrentRow(0);

    connect( ui->lista, SIGNAL(currentItemChanged(QListWidgetItem*,QListWidgetItem*)), this, SLOT(raporttiValittu(QListWidgetItem*)));
    connect( ui->tulostaNappi, SIGNAL(clicked(bool)), this, SLOT(tulosta()));
    connect( ui->esikatseleNappi, SIGNAL(clicked(bool)), this, SLOT(esikatsele()));

}

void RaporttiSivu::siirrySivulle()
{
    if( !nykyraportti)
        raporttiValittu( ui->lista->item(0));
    else
        nykyraportti->alustaLomake();
}

void RaporttiSivu::raporttiValittu(QListWidgetItem *item)
{

    nykyraportti = raportit[ item->data(RAPORTTIID).toInt()];

    ui->pino->setCurrentWidget(nykyraportti);
    nykyraportti->alustaLomake();

    ui->esikatseleNappi->setVisible( nykyraportti->onkoTulostettava());
    ui->tulostaNappi->setVisible( nykyraportti->onkoTulostettava());

}

void RaporttiSivu::tulosta()
{
    if( !nykyraportti )
        return;

    QPrintDialog printDialog( &printer, this );
    if( printDialog.exec())
    {
        nykyraportti->raportti().tulosta( &printer, ui->raitaCheck->isChecked());
    }
}

void RaporttiSivu::esikatsele()
{
    // Luo tilapäisen pdf-tiedoston
    QTemporaryFile *file = new QTemporaryFile(QDir::tempPath() + "/raportti-XXXXXX.pdf", this);
    file->open();
    file->close();

    printer.setOutputFileName( file->fileName() );
    nykyraportti->raportti().tulosta(&printer, ui->raitaCheck->isChecked());
    QDesktopServices::openUrl( QUrl(file->fileName()) );

}


void RaporttiSivu::lisaaRaportti(Raportti *raportti)
{
    QListWidgetItem *item = new QListWidgetItem(raportti->raporttinimi(), ui->lista);
    item->setData(RAPORTTIID, QVariant( raportit.count()));
    item->setIcon(raportti->kuvake());
    raportit.append(raportti);
    ui->pino->addWidget(raportti);

}
