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
#include "naytinikkuna.h"
#include "naytinview.h"
#include "db/kirjanpito.h"

#include <QSettings>
#include <QAction>
#include <QToolButton>
#include <QToolBar>
#include <QMenu>
#include <QPageSetupDialog>

#include <QMimeData>
#include <QApplication>
#include <QClipboard>
#include <QFileDialog>
#include <QMessageBox>
#include <QDesktopServices>

#include <QDialog>
#include "ui_csvvientivalinnat.h"


NaytinIkkuna::NaytinIkkuna(QWidget *parent) : QMainWindow(parent)
{
    setAttribute(Qt::WA_DeleteOnClose);

    QSettings settings;
    resize(800,600);
    restoreGeometry( settings.value("NaytinIkkuna").toByteArray());

    view_ = new NaytinView(this);
    connect( view_, &NaytinView::sisaltoVaihtunut, this, &NaytinIkkuna::sisaltoMuuttui);

    setCentralWidget(view_);

    teeToolbar();

}

NaytinIkkuna::~NaytinIkkuna()
{
    QSettings settings;
    settings.setValue("NaytinIkkuna", saveGeometry());
}

void NaytinIkkuna::naytaRaportti(RaportinKirjoittaja raportti)
{
    NaytinIkkuna *ikkuna = new NaytinIkkuna;
    ikkuna->show();
    ikkuna->view()->nayta(raportti);
}

void NaytinIkkuna::sisaltoMuuttui(const QString& tyyppi)
{
    setWindowTitle( view()->otsikko() );
    csvAktio_->setEnabled( view()->csvKaytossa() );

    // Tyypin mukaan napit
    raitaAktio_->setEnabled( tyyppi == "raportti");

    if( view()->tiedostoPaate() == "jpg"  )
        avaaAktio_->setIcon( QIcon(":/pic/kuva.png"));
    else if( view()->tiedostoPaate() == "pdf")
        avaaAktio_->setIcon( QIcon(":/pic/pdf.png"));
}

void NaytinIkkuna::csvAsetukset()
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

void NaytinIkkuna::csvLeikepoydalle()
{
    qApp->clipboard()->setText( view()->csv() );
}

void NaytinIkkuna::tallennaCsv()
{
    QString polku = QFileDialog::getSaveFileName(this, tr("Vie csv-tiedostoon"),
                                                 QDir::homePath(), "csv-tiedosto (*.csv)");
    if( !polku.isEmpty())
    {
        QFile tiedosto( polku );
        if( !tiedosto.open( QIODevice::WriteOnly))
        {
            QMessageBox::critical(this, tr("Tiedoston vieminen"),
                                  tr("Tiedostoon %1 kirjoittaminen epäonnistui.").arg(polku));
            return;
        }
        tiedosto.write( view()->csv() );
    }
}

void NaytinIkkuna::tallenna()
{
    QString polku = QFileDialog::getSaveFileName(this, tr("Tallenna tiedostoon"),
                                                 QDir::homePath(), view()->tiedostonMuoto() );
    if( !polku.isEmpty())
    {
        QFile tiedosto( polku );
        if( !tiedosto.open( QIODevice::WriteOnly))
        {
            QMessageBox::critical(this, tr("Tiedoston tallentaminen"),
                                  tr("Tiedostoon %1 kirjoittaminen epäonnistui.").arg(polku));
            return;
        }
        tiedosto.write( view()->data() );
    }
}

void NaytinIkkuna::avaaOhjelmalla()
{
    // Luo tilapäisen pdf-tiedoston
    QString tiedostonnimi = kp()->tilapainen( QString("raportti-XXXX.").append(view()->tiedostoPaate()) );

    QFile tiedosto( tiedostonnimi);
    tiedosto.open( QIODevice::WriteOnly);
    tiedosto.write( view()->data());
    tiedosto.close();

    if( !QDesktopServices::openUrl( QUrl(tiedosto.fileName()) ))
        QMessageBox::critical(this, tr("Tiedoston avaaminen"), tr("%1-tiedostoja näyttävän ohjelman käynnistäminen ei onnistunut").arg( view()->tiedostoPaate() ) );
}

void NaytinIkkuna::sivunAsetukset()
{
    QPageSetupDialog dlg(kp()->printer(), this);
    dlg.exec();
    view()->sivunAsetuksetMuuttuneet();
}


void NaytinIkkuna::teeToolbar()
{
    QToolBar *tb = addToolBar(tr("Ikkuna"));
    tb->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);

    tb->addAction(QIcon(":/pic/peru.png"), tr("Sulje"), this, SLOT(close()));
    tb->addSeparator();

    avaaAktio_ = tb->addAction(QIcon(":/pic/pdf.png"), tr("Avaa"), this, SLOT(avaaOhjelmalla()) );
    tb->addAction(QIcon(":/pic/tiedostoon.png"), tr("Tallenna"), this, SLOT(tallenna()));

    tb->addSeparator();

    raitaAktio_ = tb->addAction(QIcon(":/pic/raidoitus.png"), tr("Raidat"));
    raitaAktio_->setCheckable(true);

    tb->addAction(QIcon(":/pic/sivunasetukset.png"), tr("Sivun asetukset"), this, SLOT(sivunAsetukset()));
    tb->addAction(QIcon(":/pic/tulosta.png"), tr("Tulosta"));

    tb->addSeparator();

    htmlAktio_ = new QAction(QIcon(":/pic/web.png"), tr("HTML") );
    tb->addAction(htmlAktio_);

    csvAktio_ = new QAction(QIcon(":/pic/csv.png"), tr("CSV") );
    tb->addAction(csvAktio_);
    QToolButton *csvBtn = dynamic_cast<QToolButton*>( tb->widgetForAction(csvAktio_) );

    QMenu *csvValikko = new QMenu();

    QAction *csvLeikepoydelleAktio = new QAction( QIcon(":/pic/csv.png"), tr("Leikepöydälle") );
    connect( csvLeikepoydelleAktio, &QAction::triggered,  this, &NaytinIkkuna::csvLeikepoydalle );
    csvValikko->addAction(csvLeikepoydelleAktio);
    QAction *csvTallennaAktio = new QAction( QIcon(":/pic/tiedostoon.png"), tr("Tiedostoon"));
    connect( csvTallennaAktio, &QAction::triggered, this, &NaytinIkkuna::tallennaCsv);
    csvValikko->addAction(csvTallennaAktio);
    csvValikko->addSeparator();

    QAction *csvAsetukset = new QAction( QIcon(":/pic/ratas.png"), tr("CSV:n muoto"));
    connect( csvAsetukset, &QAction::triggered, this, &NaytinIkkuna::csvAsetukset);
    csvValikko->addAction(csvAsetukset);

    csvBtn->setMenu(csvValikko);

}

