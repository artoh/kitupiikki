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
#include <QFileInfo>

#include <QVBoxLayout>

#include <QDialog>
#include "ui_csvvientivalinnat.h"
#include <QAction>

#include <QShortcut>
#include <QMouseEvent>
#include <QMenu>

#include <QDebug>

#include <QImage>
#include <QApplication>

#include <QSettings>

#include "db/kpkysely.h"

#include "naytin/raporttinaytin.h"
#include "naytin/laaditunraportinnaytin.h"
#include "naytin/pdfnaytin.h"

#include "naytin/scenenaytin.h"
#include "naytin/tekstinaytin.h"

#include "naytin/kuvaview.h"

#include "tuonti/csvtuonti.h"
#include "naytin/esikatselunaytin.h"

NaytinView::NaytinView(QWidget *parent)
    : QWidget(parent),
      leiska_{ new (QVBoxLayout)}
{
    setLayout(leiska_);

    zoomAktio_ = new QAction( QIcon(":/pic/zoom-fit-width.png"), tr("Sovita leveyteen"),this);
    zoomAktio_->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_0));
    connect( zoomAktio_, &QAction::triggered, this, &NaytinView::zoomFit);

    zoomInAktio_ = new QAction( QIcon(":/pic/zoom-in.png"), tr("Suurenna"), this);
    zoomInAktio_->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_Plus));
    connect( zoomInAktio_, &QAction::triggered, this, &NaytinView::zoomIn);

    zoomOutAktio_ = new QAction( QIcon(":/pic/zoom-out.png"), tr("Pienennä"), this);
    zoomOutAktio_->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_Minus));
    connect( zoomOutAktio_, &QAction::triggered, this, &NaytinView::zoomOut);

    tulostaAktio_ = new QAction( QIcon(":/pic/tulosta.png"), tr("Tulosta"), this);
    connect( tulostaAktio_, &QAction::triggered, this, &NaytinView::tulosta);

    tallennaAktio_ = new QAction( QIcon(":/pic/tiedostoon.png"), tr("Tallenna"), this);
    connect( tallennaAktio_, &QAction::triggered, this, &NaytinView::tallenna);

    avaaAktio_ = new QAction( QIcon(":/pic/pdf.png"), tr("Avaa"), this);
    connect( avaaAktio_, &QAction::triggered, this, &NaytinView::avaaOhjelmalla);
}

NaytinView::~NaytinView()
{
    if( naytin_)
        delete naytin_;
}


void NaytinView::nayta(const QByteArray &data, bool salliPudotus)
{
    if( data.startsWith("%PDF"))
    {        
        vaihdaNaytin( new Naytin::PdfNaytin(data));
//        Naytin::PdfView* view = new Naytin::PdfView(data);
//        view->salliPudotus(salliPudotus);
//        vaihdaNaytin( new Naytin::SceneNaytin( view ));
    }
    else {
        QImage kuva;
        kuva.loadFromData(data);
        if( !kuva.isNull()) {
            Naytin::KuvaView* view = new Naytin::KuvaView(kuva);
            view->salliPudotus(salliPudotus);
            vaihdaNaytin( new Naytin::SceneNaytin( view ));
        } else {
            QString tyyppi = KpKysely::tiedostotyyppi(data);
            if(tyyppi != "application/octet-stream") {
                vaihdaNaytin( new Naytin::TekstiNaytin( Tuonti::CsvTuonti::haistettuKoodattu(data) ) );
            } else {
                vaihdaNaytin(nullptr);
            }
        }
    }
}

void NaytinView::nayta(RaportinKirjoittaja raportti)
{
    vaihdaNaytin( new Naytin::RaporttiNaytin(raportti, this) );
    zoomFit();
}

void NaytinView::nayta(const RaporttiValinnat &valinnat)
{
    vaihdaNaytin( new Naytin::LaaditunRaportinNaytin(this, valinnat));
}

void NaytinView::nayta(const QString &teksti)
{
    vaihdaNaytin( new Naytin::TekstiNaytin(teksti, this) );
}

Naytin::EsikatseluNaytin* NaytinView::esikatsele(Esikatseltava *katseltava)
{
    Naytin::EsikatseluNaytin *naytin = new Naytin::EsikatseluNaytin(katseltava, nullptr);
    vaihdaNaytin( naytin );
    return naytin;
}

void NaytinView::virkista()
{
    if( naytin_)
        naytin_->virkista();
}



void NaytinView::paivita()
{
    if( naytin_)
        naytin_->paivita();
}

void NaytinView::raidoita(bool raidat)
{
    if( naytin_)
        naytin_->raidoita(raidat);
}

void NaytinView::tulosta()
{
    QPrintDialog printDialog( printer(), this);
    printDialog.setOptions( QPrintDialog::PrintToFile | QPrintDialog::PrintShowPageSize );
    if( printDialog.exec() && naytin_)
    {
        naytin_->tulosta( printer() );
    }

}

void NaytinView::sivunAsetukset()
{
    QPageSetupDialog dlg(naytin_ ? naytin_->printer() : kp()->printer(), this);
    dlg.exec();
    if(naytin_) {
        naytin_->asetaSuunta(dlg.printer()->pageLayout().orientation() == QPageLayout::Portrait ? QPageLayout::Portrait : QPageLayout::Landscape);
    }
    paivita();
}

void NaytinView::avaaOhjelmalla()
{
    // Luo tilapäisen tiedoston
    QString tiedostonnimi = kp()->tilapainen( QString("liite-XXXX.").append(tiedostoPaate()) );

    QFile tiedosto( tiedostonnimi);
    tiedosto.open( QIODevice::WriteOnly);    
    tiedosto.write( data());
    tiedosto.close();

    if( !QDesktopServices::openUrl( QUrl::fromLocalFile(tiedosto.fileName()) ))
        QMessageBox::critical(this, tr("Tiedoston avaaminen"), tr("%1-tiedostoja näyttävän ohjelman käynnistäminen ei onnistunut").arg( tiedostoPaate() ) );
}

void NaytinView::tallenna()
{        

    QString polku = QFileDialog::getSaveFileName(this, tr("Tallenna tiedostoon"),
                                                 raporttipolku(), tiedostonMuoto() );
    if( !polku.isEmpty())
    {
        setRaporttipolku(polku);
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
    QString tiedostonnimi = kp()->tilapainen( "XXXX.html" );

    QFile tiedosto( tiedostonnimi);
    tiedosto.open( QIODevice::WriteOnly);

    QTextStream out( &tiedosto);
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    out.setCodec("utf8");
#endif

    out << html();
    tiedosto.close();

    Kirjanpito::avaaUrl(QUrl::fromLocalFile(tiedostonnimi));
}

void NaytinView::htmlLeikepoydalle()
{
    QMimeData *mimeData = new QMimeData;
    QString data = html();
    mimeData->setHtml( data );
    mimeData->setText( data );

    qApp->clipboard()->setMimeData(mimeData);
    emit kp()->onni(tr("Viety leikepöydälle"), Kirjanpito::Onnistui);

}

void NaytinView::tallennaHtml()
{
    QString polku = QFileDialog::getSaveFileName(this, tr("Tallenna tiedostoon"),
                                                 raporttipolku(), "html-tiedosto (*.html)");
    if( !polku.isEmpty())
    {
        setRaporttipolku(polku);
        QFile tiedosto( polku );
        if( !tiedosto.open( QIODevice::WriteOnly))
        {
            QMessageBox::critical(this, tr("Tiedoston vieminen"),
                                  tr("Tiedostoon %1 kirjoittaminen epäonnistui.").arg(polku));
            return;
        }
        QTextStream out( &tiedosto);        
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
        out.setCodec("utf8");
#endif

        out << html();
        tiedosto.close();
    }
}

void NaytinView::csvAsetukset()
{
    QChar erotin = kp()->settings()->value("CsvErotin", QChar(',')).toChar();
    QChar despilkku = kp()->settings()->value("CsvDesimaali", QChar(',')).toChar();
    QString pvmmuoto = kp()->settings()->value("CsvPaivays", "dd.MM.yyyy").toString();
    QString koodaus = kp()->settings()->value("CsvKoodaus","utf8").toString();

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
            kp()->settings()->setValue("CsvErotin", QChar(','));
        else if( ui.eropuolipiste->isChecked())
            kp()->settings()->setValue("CsvErotin", QChar(';'));
        else if( ui.erosarakain->isChecked())
            kp()->settings()->setValue("CsvErotin", QChar('\t'));

        if( ui.utf8->isChecked())
            kp()->settings()->setValue("CsvKoodaus", "utf8");
        else if( ui.latin1->isChecked())
            kp()->settings()->setValue("CsvKoodaus", "latin1");

        if( ui.desipilkku->isChecked())
            kp()->settings()->setValue("CsvDesimaali", QChar(','));
        else if( ui.desipiste->isChecked())
            kp()->settings()->setValue("CsvDesimaali", QChar('.'));

        if( ui.suomipaiva->isChecked())
            kp()->settings()->setValue("CsvPaivays","dd.MM.yyyy");
        else if( ui.isopaiva->isChecked())
            kp()->settings()->setValue("CsvPaivays", "yyyy-MM-dd");
        else if( ui.usapaiva->isChecked())
            kp()->settings()->setValue("CsvPaivays","MM/dd/yyyy");
    }
}

void NaytinView::tallennaCsv()
{
    QString polku = QFileDialog::getSaveFileName(this, tr("Vie csv-tiedostoon"),
                                                 raporttipolku(), "csv-tiedosto (*.csv)");
    if( !polku.isEmpty())
    {
        setRaporttipolku(polku);
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
    emit kp()->onni(tr("Viety leikepöydälle"));
}

void NaytinView::zoomFit()
{
    if( naytin_)
        naytin_->zoomFit();
}

void NaytinView::zoomIn()
{
    if( naytin_)
        naytin_->zoomIn();
}

void NaytinView::zoomOut()
{
    if( naytin_)
        naytin_->zoomOut();
}

QString NaytinView::raporttipolku() const
{
    return kp()->settings()->value(kp()->asetukset()->uid() + "/raporttipolku", QDir::homePath()).toString();
}

void NaytinView::setRaporttipolku(const QString &polku)
{    
    kp()->settings()->setValue( kp()->asetukset()->uid() + "/raporttipolku", QFileInfo(polku).absolutePath() );
}

QString NaytinView::otsikko() const
{
    return naytin_ ? naytin_->otsikko() : QString();
}

bool NaytinView::csvKaytossa() const
{
    return naytin_ ? naytin_->csvMuoto() : false;
}

bool NaytinView::htmlKaytossa() const
{
    return naytin_ ? naytin_->htmlMuoto() : false;
}

bool NaytinView::raidatKaytossa() const
{
    return naytin_ ? naytin_->voikoRaidoittaa() : false;
}

bool NaytinView::zoomKaytossa() const
{
    return naytin_ ? naytin_->voikoZoomata() : false;
}

bool NaytinView::paivitaKaytossa() const
{
    return naytin_ ? naytin_->voikoVirkistaa() : false;
}

QByteArray NaytinView::csv()
{
    return naytin_ ? naytin_->csv() : QByteArray();
}

QString NaytinView::tiedostonMuoto()
{
    return naytin_ ? naytin_->tiedostonMuoto() : QString();
}

QString NaytinView::tiedostoPaate()
{
    return naytin_ ? naytin_->tiedostonPaate() : QString();
}

QByteArray NaytinView::data()
{
    return naytin_ ? naytin_->data() : QByteArray();
}

QString NaytinView::html()
{
    return naytin_ ? naytin_->html() : QString();
}

void NaytinView::vaihdaNaytin(Naytin::AbstraktiNaytin *naytin)
{
    if( naytin_ )
    {
        naytin_->widget()->hide();
        leiska_->removeWidget( naytin_->widget() );
        naytin_->deleteLater();
        naytin_ = nullptr;
    }

    naytin_ = naytin;

    if(naytin) {
        leiska_->addWidget( naytin->widget());                
        connect( naytin, &Naytin::AbstraktiNaytin::otsikkoVaihtui, this, &NaytinView::otsikkoVaihtunut);        
        connect( naytin, &Naytin::AbstraktiNaytin::tiedostoPudotettu, this, &NaytinView::tiedostoPudotettu);
        connect( naytin, &Naytin::AbstraktiNaytin::eiSisaltoa, this, &NaytinView::eiSisaltoa);
    }

    qApp->processEvents();

    emit sisaltoVaihtunut();
}


void NaytinView::contextMenuEvent(QContextMenuEvent *event)
{
    QMenu valikko(this);
    if( naytin_ && naytin_->voikoZoomata() )
    {
        valikko.addAction(zoomAktio_);
        valikko.addAction(zoomInAktio_);
        valikko.addAction(zoomOutAktio_);
        valikko.addSeparator();
    }
    valikko.addAction(avaaAktio_);
    valikko.addAction(tulostaAktio_);
    valikko.addAction(tallennaAktio_);

    valikko.exec( event->globalPos() );
}

QPrinter *NaytinView::printer()
{
    return naytin_ ? naytin_->printer() : kp()->printer();
}
