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
#include "pdfview.h"

#include <poppler/qt5/poppler-qt5.h>
#include <QGraphicsPixmapItem>
#include <QPrinter>
#include <QPainter>
#include <QGraphicsSimpleTextItem>
#include "db/kirjanpito.h"
#include <QSettings>

#include "tools/pdf/pdftoolkit.h"

Naytin::PdfView::PdfView(const QByteArray &pdf) :
    data_(pdf)
{
    skaala_ = kp()->settings()->value("LiiteZoom",100).toInt() / 100.0;
}

QByteArray Naytin::PdfView::data() const
{
    return data_;
}

QString Naytin::PdfView::otsikko() const
{
    Poppler::Document *pdfDoc = Poppler::Document::loadFromData( data_ );
    QString otsikkoni = pdfDoc->info("Title") ;
    delete pdfDoc;
    return otsikkoni;
}

void Naytin::PdfView::paivita() const
{

    scene()->setBackgroundBrush(QBrush(Qt::gray));
    scene()->clear();

    PdfRendererDocument* renderer = PdfToolkit::renderer(data_);

//    Poppler::Document *pdfDoc = Poppler::Document::loadFromData( data_ );
//    if(!pdfDoc)
//        return;

    if( renderer->locked() ) {
        QGraphicsSimpleTextItem *text = scene()->addSimpleText(tr("Tiedosto on salakirjoitettu"), QFont("FreeSans",14));
        text->setBrush(QBrush(Qt::yellow));
        delete renderer;
        return;
    }

    double ypos = 0.0;
    double leveys = 0.0;
    double leveyteen = width() * skaala_ - 20.0;

    int sivut = renderer->pageCount();
    int naytettavat = sivut < 20 ? sivut : 20;

    if( naytettavat != sivut) {
        QGraphicsSimpleTextItem *text = scene()->addSimpleText(tr("Tiedostossa on %1 sivua, joista näytetään 20 ensimmäistä. Paina hiiren oikeaa nappia ja valitse Avaa nähdäksesi kokonaan.").arg(sivut), QFont("FreeSans",14));
        text->setBrush(QBrush(Qt::yellow));
        ypos += text->boundingRect().height() + 5;
    }

    // Monisivuisen pdf:n sivut pinotaan päällekkäin
    for( int sivu = 0; sivu < naytettavat; sivu++)
    {        
        QImage image = renderer->renderPageToWidth(sivu, leveyteen);
        QPixmap kuva = QPixmap::fromImage( image, Qt::DiffuseAlphaDither);

        scene()->addRect(2, ypos+2, kuva.width(), kuva.height(), QPen(Qt::NoPen), QBrush(Qt::black) );

        QGraphicsPixmapItem *item = scene()->addPixmap(kuva);
        item->setY( ypos );
        scene()->addRect(0, ypos, kuva.width(), kuva.height(), QPen(Qt::black), Qt::NoBrush );

        if( kuva.width() > leveys)
            leveys = kuva.width();

        ypos += kuva.height() + 10.0;

    }

    if( naytettavat != sivut) {
        QGraphicsSimpleTextItem *text = scene()->addSimpleText(tr("Tiedostossa on %1 sivua, joista näytetään 20 ensimmäistä.  Paina hiiren oikeaa nappia ja valitse Avaa nähdäksesi kokonaan.").arg(sivut), QFont("FreeSans",16));
        text->setBrush(QBrush(Qt::yellow));
        text->setY(ypos);
        ypos += text->boundingRect().height() + 5;
    }

    scene()->setSceneRect(-5.0, -5.0, leveys + 10.0, ypos + 5.0  );

    delete renderer;

}

void Naytin::PdfView::tulosta(QPrinter *printer) const
{
    QPainter painter(printer);
    Poppler::Document *document = Poppler::Document::loadFromData(data_);
    if( !document) {
        return;
    }

    document->setRenderHint(Poppler::Document::TextAntialiasing);
    document->setRenderHint(Poppler::Document::Antialiasing);


    int pageCount = document->numPages();
    for(int i=0; i < pageCount; i++)
    {
        Poppler::Page *page = document->page(i);
        if( !page ) {
            delete document;
            return;
        }

        double resoluutio = 300.0;
        QImage image = page->renderToImage(resoluutio, resoluutio);
        QPixmap kuva = QPixmap::fromImage( image, Qt::DiffuseAlphaDither);
        painter.drawPixmap(painter.window(), kuva);

        if( i < pageCount - 1)
            printer->newPage();

        delete page;
    }
    painter.end();
    delete document;
}

void Naytin::PdfView::zoomIn()
{
    if( skaala_ < 0.5)
        skaala_ += 0.1;
    else
        skaala_ += 0.2;
    paivita();
}

void Naytin::PdfView::zoomOut()
{
    if( skaala_ > 0.5)
        skaala_ -= 0.2;
    else if( skaala_ > 0.15)
        skaala_ -= 0.1;
    paivita();
}

void Naytin::PdfView::zoomFit()
{
    skaala_ = 1.0;
    paivita();
}


