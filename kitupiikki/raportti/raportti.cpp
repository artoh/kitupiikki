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

#include <QDate>
#include <QFont>
#include <QPen>

#include <QFile>
#include <QTemporaryFile>
#include <QUrl>
#include <QPrintDialog>
#include <QPageSetupDialog>
#include <QDesktopServices>
#include <QTextStream>

#include <QCheckBox>
#include <QPushButton>

#include <QVBoxLayout>
#include <QHBoxLayout>

#include "raportti.h"
#include "db/kirjanpito.h"


Raportti::Raportti( QWidget *parent) : QWidget(parent)
{
        raporttiWidget = new QWidget();

        raitaCheck = new QCheckBox(tr("Tulosta taustaraidat"));
        QPushButton *htmlBtn = new QPushButton( tr("Avaa &selaimessa"));
        QPushButton *sivunasetusBtn = new QPushButton( tr("Sivun &asetukset"));
        QPushButton *esikatseluBtn = new QPushButton(tr("&Esikatsele"));
        QPushButton *tulostaBtn = new QPushButton( tr("&Tulosta"));

        QHBoxLayout *nappiLeiska = new QHBoxLayout;
        nappiLeiska->addWidget(raitaCheck);
        nappiLeiska->addStretch();
        nappiLeiska->addWidget( htmlBtn);
        nappiLeiska->addWidget( sivunasetusBtn);
        nappiLeiska->addWidget(esikatseluBtn);
        nappiLeiska->addWidget(tulostaBtn);

        QVBoxLayout *paaLeiska = new QVBoxLayout;
        paaLeiska->addWidget(raporttiWidget);
        paaLeiska->addLayout(nappiLeiska);
        paaLeiska->addStretch();

        setLayout(paaLeiska);

        connect( htmlBtn, SIGNAL(clicked(bool)), this, SLOT(avaaHtml()));
        connect( sivunasetusBtn, SIGNAL(clicked(bool)), this, SLOT(sivunAsetukset()));
        connect( esikatseluBtn, SIGNAL(clicked(bool)), this, SLOT(esikatsele()) );
        connect( tulostaBtn, SIGNAL(clicked(bool)), this, SLOT(tulosta()) );
}

void Raportti::tulosta()
{
    QPrintDialog printDialog( kp()->printer(), this );
    if( printDialog.exec())
    {
        QPainter painter( kp()->printer() );
        raportti().tulosta( kp()->printer(), &painter, raitaCheck->isChecked());
    }
}

void Raportti::esikatsele()
{
    // Luo tilapäisen pdf-tiedoston
    QTemporaryFile *file = new QTemporaryFile(QDir::tempPath() + "/raportti-XXXXXX.pdf", this);
    file->open();
    file->close();

    QPrinter tulostin(QPrinter::HighResolution);
    tulostin.setPageSize(QPrinter::A4);

    tulostin.setOutputFileName( file->fileName() );
    QPainter painter( &tulostin );
    raportti().tulosta( &tulostin, &painter, raitaCheck->isChecked());
    QDesktopServices::openUrl( QUrl(file->fileName()) );
}

void Raportti::avaaHtml()
{
    // Luo tilapäisen pdf-tiedoston
    QTemporaryFile *file = new QTemporaryFile(QDir::tempPath() + "/raportti-XXXXXX.html", this);
    file->open();
    file->close();

    QFile tiedosto( file->fileName());
    tiedosto.open( QIODevice::WriteOnly);

    QTextStream out( &tiedosto);
    out.setCodec("UTF-8");

    out << raportti().html();
    tiedosto.close();

    QDesktopServices::openUrl( QUrl(file->fileName()) );
}

void Raportti::sivunAsetukset()
{
    QPageSetupDialog dlg(kp()->printer(), this);
    dlg.exec();
}



