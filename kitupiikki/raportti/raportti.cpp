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
#include <QUrl>
#include <QPrintDialog>
#include <QPageSetupDialog>
#include <QDesktopServices>
#include <QFileDialog>
#include <QTextStream>
#include <QMessageBox>

#include <QPdfWriter>

#include <QCheckBox>
#include <QPushButton>

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>

#include <QMimeData>
#include <QApplication>
#include <QClipboard>

#include <QPrinterInfo>

#include "raportti.h"
#include "db/kirjanpito.h"

#include <QSettings>


#include "tools/pdfikkuna.h"

#include "naytin/naytinikkuna.h"


Raportti::Raportti(QWidget *parent) : QWidget(parent)
{
        raporttiWidget = new QWidget();

        raitaCheck = new QCheckBox(tr("Tulosta taustaraidat"));
        QPushButton *htmlBtn = new QPushButton( QIcon(":/pic/web.png"), tr("Avaa selaimessa"));
        QPushButton *vieBtn = new QPushButton( QIcon(":/pic/vie.png"), tr("Vie leikepöydälle"));
        QPushButton *csvBtn = new QPushButton( QIcon(":/pic/csv.png"), tr("Vie csv"));
        QPushButton *csvleikeBtn = new QPushButton( QIcon(":/pic/csv.png"), tr("CSV leikepöydälle"));
        QPushButton *csvasetusBtn = new QPushButton( QIcon(":/pic/ratas.png"), tr("CSV määritykset"));
        QPushButton *sivunasetusBtn = new QPushButton(QIcon(":/pic/sivunasetukset.png"),  tr("Sivun asetukset"));
        QPushButton *esikatseluBtn = new QPushButton(QIcon(":/pic/print.png"), tr("Esikatsele"));
        QPushButton *tulostaBtn = new QPushButton( QIcon(":/pic/tulosta.png"), tr("Tulosta"));

        QHBoxLayout *nappiLeiska = new QHBoxLayout;
        nappiLeiska->addStretch();
        nappiLeiska->addWidget(esikatseluBtn);

        QVBoxLayout *paaLeiska = new QVBoxLayout;
        paaLeiska->addWidget(raporttiWidget);
        paaLeiska->addLayout(nappiLeiska);
        paaLeiska->addStretch();

        setLayout(paaLeiska);

        connect( htmlBtn, SIGNAL(clicked(bool)), this, SLOT(avaaHtml()));
        connect( vieBtn, SIGNAL(clicked(bool)), this, SLOT(leikepoydalle()));
        connect( csvBtn, SIGNAL( clicked(bool)), this, SLOT(vieCsv()));
        connect( csvleikeBtn, SIGNAL(clicked(bool)), this, SLOT(csvleike()));
        connect( csvasetusBtn, SIGNAL(clicked(bool)), this, SLOT(csvAsetukset()));
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
        painter.end();
    }
}

void Raportti::esikatsele()
{
    NaytinIkkuna::naytaRaportti( raportti() );

    // PdfIkkuna::naytaPdf( raportti().pdf( raitaCheck->isChecked() ) );
}

void Raportti::avaaHtml()
{
    // Luo tilapäisen pdf-tiedoston
    QString tiedostonnimi = kp()->tilapainen( "raportti-XXXX.html" );

    QFile tiedosto( tiedostonnimi);
    tiedosto.open( QIODevice::WriteOnly);

    QTextStream out( &tiedosto);
    out.setCodec("UTF-8");

    out << raportti().html();
    tiedosto.close();

    Kirjanpito::avaaUrl(QUrl::fromLocalFile(tiedostonnimi));
}


void Raportti::sivunAsetukset()
{
    QPageSetupDialog dlg(kp()->printer(), this);
    dlg.exec();
}

void Raportti::leikepoydalle()
{
    QMimeData *mimeData = new QMimeData;
    mimeData->setHtml( raportti().html() );

    qApp->clipboard()->setMimeData(mimeData);

    kp()->onni(tr("Raportti viety leikepöydälle"));
}





