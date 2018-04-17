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

#include <QSqlQuery>
#include <QMessageBox>
#include <QFile>
#include <QSettings>

#ifdef Q_OS_LINUX
    #include <poppler/qt5/poppler-qt5.h>
#elif defined(Q_OS_WIN)
    #include "poppler-qt5.h"
#endif

#include <QGraphicsScene>
#include <QGraphicsView>
#include <QGraphicsPixmapItem>

#include <QPrintDialog>
#include <QPrinter>
#include <QAction>
#include <QToolBar>

#include <QDesktopServices>
#include <QUrl>

#include "pdfikkuna.h"
#include "db/kirjanpito.h"


PdfIkkuna::PdfIkkuna(const QByteArray &pdfdata,  QWidget *parent) :
    QMainWindow(parent), data(pdfdata)
{
    setAttribute(Qt::WA_DeleteOnClose);

    scene = new QGraphicsScene(this);
    view = new QGraphicsView(scene);

    view->setDragMode( QGraphicsView::ScrollHandDrag);
    view->setBackgroundBrush( QBrush( Qt::lightGray ));

    QSettings settings;
    resize(800,600);
    restoreGeometry( settings.value("PdfIkkuna").toByteArray());

    QToolBar *tb = addToolBar(tr("Pdf"));
    tb->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
//    tb->addAction(QIcon(":/pic/peru.png"), tr("Sulje"), this, SLOT(close()));
    tb->addAction(QIcon(":/pic/pdf.png"), tr("Avaa"), this, SLOT(avaaOhjelmalla()));
    tb->addAction(QIcon(":/pic/tulosta.png"), tr("Tulosta"), this, SLOT(tulosta()));


    setCentralWidget(view);
}

PdfIkkuna::~PdfIkkuna()
{
    QSettings settings;
    settings.setValue("PdfIkkuna", saveGeometry());
}

void PdfIkkuna::tulosta()
{
    QPrintDialog printDialog( kp()->printer(), this );
    if( printDialog.exec())
    {
        QPainter painter( kp()->printer() );

        Poppler::Document* document = Poppler::Document::loadFromData(data);
        document->setRenderBackend(Poppler::Document::ArthurBackend);

        int pageCount = document->numPages();

        for(int i = 0; i < pageCount; i++) {
            document->page(i)->renderToPainter(&painter, kp()->printer()->resolution(), kp()->printer()->resolution(),
                                               0,0,document->page(i)->pageSize().width(),document->page(i)->pageSize().height());
            kp()->printer()->newPage();
        }
        painter.end();
    }

}

void PdfIkkuna::avaaOhjelmalla()
{
    QFile tiedosto( kp()->tilapainen("XXXX.pdf"));
    tiedosto.open( QIODevice::WriteOnly);
    tiedosto.write( data );
    tiedosto.close();

    if( !QDesktopServices::openUrl( QUrl(tiedosto.fileName()) ))
        QMessageBox::critical(this, tr("Pdf-tiedoston näyttäminen"), tr("Pdf-tiedostoja näyttävän ohjelman käynnistäminen ei onnistunut"));
}

void PdfIkkuna::naytaPdf(const QByteArray &pdfdata)
{
    PdfIkkuna *ikkuna = new PdfIkkuna( pdfdata );
    ikkuna->show();
    ikkuna->raise();
}

void PdfIkkuna::naytaLiite(const int tositeId, const int liiteId)
{
    QSqlQuery kysely( QString("SELECT data FROM liite WHERE tosite=%1 AND liiteno=%2")
                      .arg(tositeId).arg(liiteId));
    if( kysely.next() )
    {
        QByteArray data = kysely.value("data").toByteArray();
        naytaPdf(data);
    }
    else
    {
        QMessageBox::critical(0, tr("Virhe liitteen näyttämisessä"),
                              tr("Liitettä %1-%2 ei löydy").arg(tositeId).arg(liiteId));
    }
}

void PdfIkkuna::naytaPdf(const QString &tiedostonnimi)
{
    QByteArray data;
    QFile tiedosto( tiedostonnimi);
    if( tiedosto.open( QIODevice::ReadOnly) )
    {
        data = tiedosto.readAll();
        tiedosto.close();
        naytaPdf( data );
    }
    else
        QMessageBox::critical(0, tr("Virhe tiedoston näyttämisessä"),
                              tr("Tiedostoa %1 ei voi avata").arg(tiedostonnimi));
}

void PdfIkkuna::resizeEvent(QResizeEvent * /* event */)
{
    scene->clear();

    Poppler::Document *pdfDoc = Poppler::Document::loadFromData( data );
    pdfDoc->setRenderHint(Poppler::Document::TextAntialiasing);
    pdfDoc->setRenderHint(Poppler::Document::Antialiasing);

    setWindowTitle( pdfDoc->info("Title") );

    double ypos = 0.0;

    // Monisivuisen pdf:n sivut pinotaan päällekkäin
    for( int sivu = 0; sivu < pdfDoc->numPages(); sivu++)
    {
        Poppler::Page *pdfSivu = pdfDoc->page(sivu);

        if( !pdfSivu )
            continue;

        double pdfleveys = pdfSivu->pageSizeF().width();
        double viewleveys = width() - 20.0;
        double skaala = viewleveys / pdfleveys * 72.0;

        QImage image = pdfSivu->renderToImage(skaala,skaala);

        QPixmap kuva = QPixmap::fromImage( image, Qt::DiffuseAlphaDither);

        scene->addRect(2, ypos+2, kuva.width(), kuva.height(), QPen(Qt::NoPen), QBrush(Qt::black) );

        QGraphicsPixmapItem *item = scene->addPixmap(kuva);
        item->setY( ypos );
        scene->addRect(0, ypos, kuva.width(), kuva.height(), QPen(Qt::black), Qt::NoBrush );

        if( kuva.width() > leveys)
            leveys = kuva.width();

        ypos += kuva.height() + 10.0;


        delete pdfSivu;
    }

    scene->setSceneRect(-5.0, -5.0, leveys + 10.0, ypos + 5.0  );

    delete pdfDoc;
}
