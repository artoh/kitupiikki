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
#include <QFileDialog>
#include <QTextStream>
#include <QMessageBox>

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
#include <QDialog>
#include "ui_csvvientivalinnat.h"


Raportti::Raportti(bool csv, QWidget *parent) : QWidget(parent)
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

        QGridLayout *nappiLeiska = new QGridLayout;
        nappiLeiska->addWidget(raitaCheck,0,0);

        nappiLeiska->addWidget( htmlBtn,0,2);
        nappiLeiska->addWidget( vieBtn ,0,3);
        nappiLeiska->addWidget( csvBtn ,1,2);
        nappiLeiska->addWidget( csvleikeBtn, 1,3 );
        nappiLeiska->addWidget( csvasetusBtn, 1, 1);
        nappiLeiska->addWidget( sivunasetusBtn,2,1);
        nappiLeiska->addWidget(esikatseluBtn,2,2);
        nappiLeiska->addWidget(tulostaBtn,2,3);

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

        csvBtn->setEnabled(csv);
        csvleikeBtn->setEnabled(csv);
        csvasetusBtn->setEnabled(csv);
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
    QString tiedosto =  kp()->tilapainen( QString("raportti-%1.pdf").arg(Kirjanpito::satujono(8)) );

    // #88: Käytetään pdf:n luomisessakin tulostusasetuksia, jotta saadaan vaakaan taikka isompaan
    // paperikokoon
    QPrinter tulostin( QPrinter::HighResolution);
    tulostin.setPageLayout( kp()->printer()->pageLayout() );

    tulostin.setOutputFileName( tiedosto );
    QPainter painter( &tulostin );
    raportti().tulosta( &tulostin, &painter, raitaCheck->isChecked());
    painter.end();

    QDesktopServices::openUrl( QUrl(tiedosto) );
}

void Raportti::avaaHtml()
{
    // Luo tilapäisen pdf-tiedoston
    QString tiedostonnimi = kp()->tilapainen( QString("raportti-%1.html").arg(Kirjanpito::satujono(8)) );

    QFile tiedosto( tiedostonnimi);
    tiedosto.open( QIODevice::WriteOnly);

    QTextStream out( &tiedosto);
    out.setCodec("UTF-8");

    out << raportti().html();
    tiedosto.close();

    QDesktopServices::openUrl( QUrl(tiedostonnimi) );
}

void Raportti::vieCsv()
{
    QString polku = QFileDialog::getSaveFileName(this, tr("Vie csv-tiedostoon"),
                                                 QDir::homePath(), "csv-tiedosto (csv.*)");
    if( !polku.isEmpty())
    {
        QFile tiedosto( polku );
        if( !tiedosto.open( QIODevice::WriteOnly))
        {
            QMessageBox::critical(this, tr("Tiedoston vieminen"),
                                  tr("Tiedostoon %1 kirjoittaminen epäonnistui.").arg(polku));
            return;
        }
        tiedosto.write( raportti(true).csv() );
    }
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

void Raportti::csvleike()
{
    // Viedään tekstinä, koska mimetyyppi text/csv huonosti tuettu

    qApp->clipboard()->setText( raportti(true).csv());

    kp()->onni(tr("Raportti viety leikepöydälle"));
}

void Raportti::csvAsetukset()
{
    QSettings settings;
    QChar erotin = settings.value("CsvErotin", QChar(',')).toChar();
    QChar despilkku = settings.value("CsvDesimaali", QChar(',')).toChar();
    QString pvmmuoto = settings.value("CsvPaivays", "dd.MM.yyyy").toString();
    QString koodaus = settings.value("CsvKoodaus","utf8").toString();

    QDialog dlg;
    Ui::CsvVientiValintaDlg ui;
    ui.setupUi(&dlg);

    ui.eropilkku->setChecked( erotin == ',' );
    ui.eropuolipiste->setChecked( erotin == ';' );
    ui.erosarakain->setChecked( erotin == '\t' );

    ui.utf8->setChecked( koodaus == "utf8");
    ui.latin1->setChecked( koodaus == "latin1");

    ui.desipilkku->setChecked( despilkku == ',');
    ui.desipiste->setChecked( despilkku == '.');

    ui.suomipaiva->setChecked( pvmmuoto == "dd.MM.yyyy");
    ui.isopaiva->setChecked( pvmmuoto == "yyyy-MM-dd");
    ui.usapaiva->setChecked( pvmmuoto == "MM/dd/yyyy");

    if( dlg.exec())
    {
        if( ui.eropilkku->isChecked())
            settings.setValue("CsvErotin", QChar(','));
        else if( ui.eropuolipiste->isChecked())
            settings.setValue("CsvErotin", QChar(';'));
        else if( ui.erosarakain->isChecked())
            settings.setValue("CsvErotin", QChar('\t'));

        if( ui.utf8->isChecked())
            settings.setValue("CsvKoodaus", "utf8");
        else if( ui.latin1->isChecked())
            settings.setValue("CsvKoodaus", "latin1");

        if( ui.desipilkku->isChecked())
            settings.setValue("CsvDesimaali", QChar(','));
        else if( ui.desipiste->isChecked())
            settings.setValue("CsvDesimaali", QChar('.'));

        if( ui.suomipaiva->isChecked())
            settings.setValue("CsvPaivays","dd.MM.yyyy");
        else if( ui.isopaiva->isChecked())
            settings.setValue("CsvPaivays", "yyyy-MM-dd");
        else if( ui.usapaiva->isChecked())
            settings.setValue("CsvPaivays","MM/dd/yyyy");
    }
}



