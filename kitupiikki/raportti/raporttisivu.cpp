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

#include <QHBoxLayout>
#include <QListWidget>
#include <QListWidgetItem>

#include "raporttisivu.h"

#include "paivakirjaraportti.h"
#include "db/kirjanpito.h"

#include "raporittitesti.h"

RaporttiSivu::RaporttiSivu(QWidget *parent) : KitupiikkiSivu(parent),
    nykyinen(0), printer(QPrinter::HighResolution)
{
    lista = new QListWidget;

    wleiska = new QHBoxLayout;

    QHBoxLayout *paaleiska = new QHBoxLayout;
    paaleiska->addWidget(lista,0);
    paaleiska->addLayout(wleiska, 1);

    setLayout(paaleiska);

    connect( lista, SIGNAL(currentItemChanged(QListWidgetItem*,QListWidgetItem*)), this, SLOT(raporttiValittu(QListWidgetItem*)));


/*
    ui = new Ui::RaporttiWg();
    ui->setupUi(this);

    lisaaRaportti(new PaivakirjaRaportti);

    lisaaRaportti(new RaporttiTesti("Tuloslaskelma"));

    ui->lista->setCurrentRow(0);

    connect( ui->lista, SIGNAL(currentItemChanged(QListWidgetItem*,QListWidgetItem*)), this, SLOT(raporttiValittu(QListWidgetItem*)));
    connect( ui->tulostaNappi, SIGNAL(clicked(bool)), this, SLOT(tulosta()));
    connect( ui->esikatseleNappi, SIGNAL(clicked(bool)), this, SLOT(esikatsele()));
*/

}

void RaporttiSivu::siirrySivulle()
{
    // Laitetaan valinnat listaan
    lista->clear();

    lisaaRaportti("Päiväkirja","Päiväkirja",":/pic/Paivakirja64.png");

    // Lisätään muokattavat raportit
    foreach (QString rnimi, kp()->asetukset()->avaimet("Raportti/") )
    {
        lisaaRaportti( rnimi.mid(9), rnimi);
    }
}

bool RaporttiSivu::poistuSivulta()
{
    if( nykyinen )
    {
        wleiska->removeWidget( nykyinen );
        delete( nykyinen );
        nykyinen = 0;
    }
    return true;

}

void RaporttiSivu::raporttiValittu(QListWidgetItem *item)
{
    if( nykyinen )
    {
        wleiska->removeWidget( nykyinen );
        delete( nykyinen );
        nykyinen = 0;
    }


    QString raporttinimi = item->data(RAPORTTINIMI).toString();

    if( raporttinimi == "Päiväkirja")
        nykyinen = new PaivakirjaRaportti();
    else if( raporttinimi.startsWith("Raportti/"))
        nykyinen = new RaporttiTesti( raporttinimi.mid(9));


    if( nykyinen )
        wleiska->addWidget(nykyinen);

/*
    nykyraportti = raportit[ item->data(RAPORTTIID).toInt()];

    ui->pino->setCurrentWidget(nykyraportti);
    nykyraportti->alustaLomake();

    ui->esikatseleNappi->setVisible( nykyraportti->onkoTulostettava());
    ui->tulostaNappi->setVisible( nykyraportti->onkoTulostettava());
*/

}

void RaporttiSivu::lisaaRaportti(const QString &otsikko, const QString &nimi, const QString &kuvakenimi)
{
    QListWidgetItem *uusi = new QListWidgetItem( QIcon(kuvakenimi), otsikko, lista);
    uusi->setData( RAPORTTINIMI, nimi);
}
/*
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
*/
/*
void RaporttiSivu::lisaaRaportti(Raportti *raportti)
{
    QListWidgetItem *item = new QListWidgetItem(raportti->raporttinimi(), ui->lista);
    item->setData(RAPORTTIID, QVariant( raportit.count()));
    item->setIcon(raportti->kuvake());
    raportit.append(raportti);
    ui->pino->addWidget(raportti);

}

*/
