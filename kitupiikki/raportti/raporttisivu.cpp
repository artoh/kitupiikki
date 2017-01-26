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

#include <QUiLoader>
#include <QFile>
#include <QTemporaryFile>
#include <QUrl>

#include <QPrintDialog>
#include <QDesktopServices>

#include "raporttisivu.h"

#include "paivakirjaraportti.h"
#include "db/kirjanpito.h"

RaporttiSivu::RaporttiSivu(QWidget *parent) : QWidget(parent),
    nykyraportti(0), printer(QPrinter::HighResolution)
{
    ui = new Ui::RaporttiWg();
    ui->setupUi(this);

    lisaaRaportti(new PaivakirjaRaportti);

    connect( Kirjanpito::db(), SIGNAL(tietokantaVaihtui()), this, SLOT(tyhjenna()));
    connect( ui->lista, SIGNAL(currentItemChanged(QListWidgetItem*,QListWidgetItem*)), this, SLOT(raporttiValittu(QListWidgetItem*)));
    connect( ui->tulostaNappi, SIGNAL(clicked(bool)), this, SLOT(tulosta()));
    connect( ui->esikatseleNappi, SIGNAL(clicked(bool)), this, SLOT(esikatsele()));

}

void RaporttiSivu::raporttiValittu(QListWidgetItem *item)
{
    lataaUi( item->data(LOMAKE).toString());
    nykyraportti = raportit[ item->data(RAPORTTIID).toInt()];
    nykyraportti->alustaLomake(ui->kehys);

}

void RaporttiSivu::tulosta()
{
    if( !nykyraportti )
        return;

    QPrintDialog printDialog( &printer, this );
    if( printDialog.exec())
    {
        nykyraportti->tulosta(&printer, ui->kehys);
    }
}

void RaporttiSivu::esikatsele()
{
    // Luo tilapäisen pdf-tiedoston
    QTemporaryFile *file = new QTemporaryFile(QDir::tempPath() + "/raportti-XXXXXX.pdf", this);
    file->open();
    file->close();

    printer.setOutputFileName( file->fileName() );
    nykyraportti->tulosta(&printer, ui->kehys);
    QDesktopServices::openUrl( QUrl(file->fileName()) );

}

void RaporttiSivu::tyhjenna()
{
    if( nykyraportti )
        nykyraportti->alustaLomake(ui->kehys);
}

void RaporttiSivu::lataaUi(const QString &uinimi)
{
    QUiLoader loader;
    QFile tiedosto( QString(":/raportti/%1.ui").arg(uinimi));
    tiedosto.open( QFile::ReadOnly);

    QWidget *widget = loader.load(&tiedosto, this);
    QHBoxLayout *leiska = new QHBoxLayout;
    leiska->addWidget(widget);
    ui->kehys->setLayout(leiska);

}

void RaporttiSivu::lisaaRaportti(Raportti *raportti)
{
    QListWidgetItem *item = new QListWidgetItem(raportti->raporttinimi(), ui->lista);
    item->setData(LOMAKE, QVariant(raportti->lomake() ));
    item->setData(RAPORTTIID, QVariant( raportit.count()));
    item->setIcon(raportti->kuvake());
    raportit.append(raportti);
}
