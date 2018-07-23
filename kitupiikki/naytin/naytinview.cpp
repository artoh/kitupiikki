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
#include "naytinview.h"

#include "pdfscene.h"
#include "kuvanaytin.h"
#include "raporttiscene.h"
#include "db/kirjanpito.h"

#include <QPageSetupDialog>

#include <QSettings>
#include <QMimeData>
#include <QApplication>
#include <QClipboard>
#include <QFileDialog>
#include <QMessageBox>
#include <QDesktopServices>
#include <QPrintDialog>
#include <QTextStream>

#include <QDialog>
#include "ui_csvvientivalinnat.h"
#include <QAction>

#include <QShortcut>
#include <QMouseEvent>
#include <QMenu>

NaytinView::NaytinView(QWidget *parent)
    : QGraphicsView(parent)
{

    zoomAktio_ = new QAction( QIcon(":/pic/zoom-fit-width.png"), tr("Sovita leveyteen"));
    zoomAktio_->setShortcut(QKeySequence(Qt::CTRL + Qt::Key_0));
    connect( zoomAktio_, &QAction::triggered, [this] { this->zoomaus_ = 1.00; this->paivita(); }  );

    zoomInAktio_ = new QAction( QIcon(":/pic/zoom-in.png"), tr("Suurenna"));
    zoomInAktio_->setShortcut(QKeySequence(Qt::CTRL + Qt::Key_Plus));
    connect( zoomInAktio_, &QAction::triggered, [this] { this->zoomaus_ *= 1.5; this->paivita(); } );

    zoomOutAktio_ = new QAction( QIcon(":/pic/zoom-out.png"), tr("Pienennä"));
    zoomOutAktio_->setShortcut(QKeySequence(Qt::CTRL + Qt::Key_Minus));
    connect( zoomOutAktio_, &QAction::triggered, [this] { this->zoomaus_ *= 0.5; this->paivita();} );

    tulostaAktio_ = new QAction( QIcon(":/pic/tulosta.png"), tr("Tulosta"));
    connect( tulostaAktio_, &QAction::triggered, this, &NaytinView::tulosta);

    tallennaAktio_ = new QAction( QIcon(":/pic/tiedostoon.png"), tr("Tallenna"));
    connect( tallennaAktio_, &QAction::triggered, this, &NaytinView::tallenna);

}


void NaytinView::nayta(const QByteArray &data)
{
    if( data.startsWith("%PDF"))
        vaihdaScene( new PdfScene(data, this) );
    else
        vaihdaScene( new KuvaNaytin(data, this));

}

void NaytinView::nayta(RaportinKirjoittaja raportti)
{
    vaihdaScene( new RaporttiScene(raportti)  );
}

void NaytinView::sivunAsetuksetMuuttuneet()
{
    if( scene_->sivunAsetuksetMuuttuneet() )
        paivita();
}

void NaytinView::paivita()
{
    scene_->piirraLeveyteen( zoomaus_ * width() - 20.0 );
}

void NaytinView::raidoita(bool raidat)
{
    if( scene_->raidoita(raidat))
        paivita();
}

void NaytinView::tulosta()
{
    QPrintDialog printDialog( kp()->printer(), this);
    if( printDialog.exec())
        scene_->tulosta( kp()->printer() );

}

void NaytinView::sivunAsetukset()
{
    QPageSetupDialog dlg(kp()->printer(), this);
    dlg.exec();
    sivunAsetuksetMuuttuneet();
}

void NaytinView::avaaOhjelmalla()
{
    // Luo tilapäisen pdf-tiedoston
    QString tiedostonnimi = kp()->tilapainen( QString("raportti-XXXX.").append(tiedostoPaate()) );

    QFile tiedosto( tiedostonnimi);
    tiedosto.open( QIODevice::WriteOnly);
    tiedosto.write( data());
    tiedosto.close();

    if( !QDesktopServices::openUrl( QUrl(tiedosto.fileName()) ))
        QMessageBox::critical(this, tr("Tiedoston avaaminen"), tr("%1-tiedostoja näyttävän ohjelman käynnistäminen ei onnistunut").arg( tiedostoPaate() ) );
}

void NaytinView::tallenna()
{
    QString polku = QFileDialog::getSaveFileName(this, tr("Tallenna tiedostoon"),
                                                 QDir::homePath(), tiedostonMuoto() );
    if( !polku.isEmpty())
    {
        QFile tiedosto( polku );
        if( !tiedosto.open( QIODevice::WriteOnly))
        {
            QMessageBox::critical(this, tr("Tiedoston tallentaminen"),
                                  tr("Tiedostoon %1 kirjoittaminen epäonnistui.").arg(polku));
            return;
        }
        tiedosto.write( data() );
    }
}

void NaytinView::avaaHtml()
{
    QString tiedostonnimi = kp()->tilapainen( "raportti-XXXX.html" );

    QFile tiedosto( tiedostonnimi);
    tiedosto.open( QIODevice::WriteOnly);

    QTextStream out( &tiedosto);
    out.setCodec("UTF-8");

    out << html();
    tiedosto.close();

    Kirjanpito::avaaUrl(QUrl::fromLocalFile(tiedostonnimi));
}

void NaytinView::htmlLeikepoydalle()
{
    QMimeData *mimeData = new QMimeData;
    mimeData->setHtml( html() );

    qApp->clipboard()->setMimeData(mimeData);
    kp()->onni(tr("Viety leikepöydälle"));

}

void NaytinView::tallennaHtml()
{
    QString polku = QFileDialog::getSaveFileName(this, tr("Tallenna tiedostoon"),
                                                 QDir::homePath(), "html-tiedosto (*.html)");
    if( !polku.isEmpty())
    {
        QFile tiedosto( polku );
        if( !tiedosto.open( QIODevice::WriteOnly))
        {
            QMessageBox::critical(this, tr("Tiedoston vieminen"),
                                  tr("Tiedostoon %1 kirjoittaminen epäonnistui.").arg(polku));
            return;
        }
        QTextStream out( &tiedosto);
        out.setCodec("UTF-8");

        out << html();
        tiedosto.close();
    }
}

void NaytinView::csvAsetukset()
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

void NaytinView::tallennaCsv()
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
        tiedosto.write( csv() );
    }
}

void NaytinView::csvLeikepoydalle()
{
    qApp->clipboard()->setText( csv() );
    kp()->onni(tr("Viety leikepöydälle"));
}

QString NaytinView::otsikko() const
{
    return scene_->otsikko();
}

bool NaytinView::csvKaytossa() const
{
    return scene_->csvMuoto();
}

QByteArray NaytinView::csv()
{
    return scene_->csv();
}

QString NaytinView::tiedostonMuoto()
{
    return scene_->tiedostonMuoto();
}

QString NaytinView::tiedostoPaate()
{
    return scene_->tiedostoPaate();
}

QByteArray NaytinView::data()
{
    return scene_->data();
}

QString NaytinView::html()
{
    return  scene_->html();
}

void NaytinView::vaihdaScene(NaytinScene *uusi)
{
    if( scene_)
        scene_->deleteLater();

    scene_ = uusi;

    emit( sisaltoVaihtunut(scene_->tyyppi()));

    setScene(uusi);
    paivita();
}

void NaytinView::resizeEvent(QResizeEvent * /*event*/)
{
    if( scene_)
        scene_->piirraLeveyteen( zoomaus_ * width() - 20.0);
}

void NaytinView::mousePressEvent(QMouseEvent *event)
{
    if( event->button() == Qt::RightButton)
    {
        QMenu valikko;
        valikko.addAction(zoomAktio_);
        valikko.addAction(zoomInAktio_);
        valikko.addAction(zoomOutAktio_);
        valikko.addSeparator();
        valikko.addAction(tulostaAktio_);
        valikko.addAction(tallennaAktio_);

        valikko.exec(QCursor::pos());
    }
}
