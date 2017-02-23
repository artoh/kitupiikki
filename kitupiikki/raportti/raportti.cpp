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
#include <QDesktopServices>

#include <QCheckBox>
#include <QPushButton>

#include <QVBoxLayout>
#include <QHBoxLayout>

#include "raportti.h"
#include "db/kirjanpito.h"


Raportti::Raportti(QPrinter *printer, QWidget *parent) : QWidget(parent),
    tulostin(printer)
{
        raporttiWidget = new QWidget();

        raitaCheck = new QCheckBox(tr("Tulosta taustaraidat"));
        QPushButton *esikatseluBtn = new QPushButton(tr("&Esikatsele"));
        QPushButton *tulostaBtn = new QPushButton( tr("&Tulosta"));

        QHBoxLayout *nappiLeiska = new QHBoxLayout;
        nappiLeiska->addWidget(raitaCheck);
        nappiLeiska->addStretch();
        nappiLeiska->addWidget(esikatseluBtn);
        nappiLeiska->addWidget(tulostaBtn);

        QVBoxLayout *paaLeiska = new QVBoxLayout;
        paaLeiska->addWidget(raporttiWidget);
        paaLeiska->addLayout(nappiLeiska);
        paaLeiska->addStretch();

        setLayout(paaLeiska);

        connect( esikatseluBtn, SIGNAL(clicked(bool)), this, SLOT(esikatsele()) );
        connect( tulostaBtn, SIGNAL(clicked(bool)), this, SLOT(tulosta()) );
}

void Raportti::tulosta()
{
    QPrintDialog printDialog( tulostin, this );
    if( printDialog.exec())
    {
        raportti().tulosta( tulostin, raitaCheck->isChecked());
    }
}

void Raportti::esikatsele()
{
    // Luo tilapäisen pdf-tiedoston
    QTemporaryFile *file = new QTemporaryFile(QDir::tempPath() + "/raportti-XXXXXX.pdf", this);
    file->open();
    file->close();

    tulostin->setOutputFileName( file->fileName() );
    raportti().tulosta( tulostin, raitaCheck->isChecked());
    QDesktopServices::openUrl( QUrl(file->fileName()) );
}



