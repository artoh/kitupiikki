/*
   Copyright (C) 2019 Arto Hyvättinen

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
#include "liitetulostaja.h"
#include "db/kirjanpito.h"

#include <QPagedPaintDevice>
#include <QPainter>

#include <poppler/qt5/poppler-qt5.h>
#include <QGraphicsPixmapItem>
#include <QImage>

bool LiiteTulostaja::tulostaLiite(QPagedPaintDevice *printer, QPainter *painter, const QByteArray &data, const QString &tyyppi, const QDate &pvm, const QString &sarja, int tunniste)
{
    if( tyyppi == "application/pdf")
        return tulostaPdfLiite(printer, painter, data, pvm, sarja, tunniste);
    if( tyyppi.startsWith("image"))
        return tulostaKuvaLiite(printer, painter, data, pvm, sarja, tunniste);

    return false;
}

bool LiiteTulostaja::tulostaPdfLiite(QPagedPaintDevice *printer, QPainter *painter, const QByteArray &data, const QDate &pvm, const QString &sarja, int tunniste)
{
    Poppler::Document *document = Poppler::Document::loadFromData(data);
    document->setRenderBackend(Poppler::Document::ArthurBackend);

    int pageCount = document->numPages();
    for(int i=0; i < pageCount; i++)
    {
        tulostaYlatunniste(painter, pvm, sarja, tunniste);

        Poppler::Page *page = document->page(i);

        double vaakaResoluutio =  printer->pageLayout().paintRect(QPageLayout::Point).width() / page->pageSizeF().width() * printer->logicalDpiX();
        double pystyResoluutio = printer->pageLayout().paintRect(QPageLayout::Point).height() / page->pageSizeF().height() * printer->logicalDpiY();

        double resoluutio = vaakaResoluutio < pystyResoluutio ? vaakaResoluutio : pystyResoluutio;

        page->renderToPainter( painter, resoluutio, resoluutio,
                                            0, 0 - painter->fontMetrics().height() * 2 ,page->pageSize().width(), page->pageSize().height());



        if( i < pageCount - 1)
            printer->newPage();

        delete page;
    }
    delete document;
    return true;
}

bool LiiteTulostaja::tulostaKuvaLiite(QPagedPaintDevice */*printer*/, QPainter *painter, const QByteArray &data, const QDate &pvm, const QString &sarja, int tunniste)
{
    tulostaYlatunniste(painter, pvm, sarja, tunniste);
    QRect rect = painter->viewport().adjusted(0,painter->fontMetrics().height()*2,0,0);

    QImage kuva = QImage::fromData(data);

    QSize size = kuva.size();
    size.scale(rect.size(), Qt::KeepAspectRatio);
    painter->save();
    painter->setViewport( rect.x(), rect.y(),
                         size.width(), size.height());
    painter->setWindow(kuva.rect());
    painter->drawImage(0, 0, kuva);
    painter->restore();

    return !kuva.isNull();
}

void LiiteTulostaja::tulostaYlatunniste(QPainter *painter, const QDate &pvm, const QString &sarja, int tunniste)
{
    int leveys = painter->window().width();
    int korkeus = painter->fontMetrics().height() * 2;

    if( kp()->asetukset()->onko("Harjoitus") && !kp()->asetukset()->onko("Demo") )
    {
        painter->save();
        painter->setPen( QPen(Qt::green));
        painter->setFont( QFont("Sans",14));
        painter->drawText(QRect(leveys / 16 * 10,0,leveys/4, korkeus ), Qt::AlignHCenter | Qt::AlignVCenter, QString("HARJOITUS") );
        painter->restore();
    }


    painter->setFont(QFont("FreeSans",10));
    QRectF rect(0, 0, leveys/3, korkeus * 2);
    painter->drawText( rect, kp()->asetus("Nimi"));

    painter->setFont(QFont("FreeSans",18, QFont::Bold));
    QRectF pvmRect(leveys/6*2, 0, leveys/3, korkeus * 2);
    painter->drawText( pvmRect, pvm.toString("dd.MM.yyyy"),QTextOption(Qt::AlignHCenter | Qt::AlignTop));


    painter->setFont(QFont("FreeSans",18, QFont::Bold));
    QRectF tunnisteRect(leveys * 3 / 4, 0, leveys / 4, korkeus);
    painter->drawText( tunnisteRect, kp()->tositeTunnus(tunniste, pvm, sarja), QTextOption(Qt::AlignRight | Qt::AlignTop) );


    painter->setFont(QFont("FreeSans",12));

}
