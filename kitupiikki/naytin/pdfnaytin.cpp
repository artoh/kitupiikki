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
#include "pdfnaytin.h"

#include <poppler/qt5/poppler-qt5.h>
#include <QPainter>
#include <QPrinter>

#include <QPrintPreviewWidget>

Naytin::PdfNaytin::PdfNaytin(const QByteArray &pdfdata, QObject *parent)
    : PrintPreviewNaytin (parent),
      data_{ pdfdata }
{
    widget_->setZoomMode(QPrintPreviewWidget::FitToWidth);
}

QString Naytin::PdfNaytin::otsikko() const
{
    Poppler::Document *pdfDoc = Poppler::Document::loadFromData( data_ );
    QString otsikko {pdfDoc->info("Title") };
    delete pdfDoc;
    return otsikko;
}

QByteArray Naytin::PdfNaytin::data() const
{
    return data_;
}

void Naytin::PdfNaytin::tulosta(QPrinter *printer) const
{
    QPainter painter(printer);

    Poppler::Document *document = Poppler::Document::loadFromData(data_);
    document->setRenderBackend(Poppler::Document::ArthurBackend);

    int pageCount = document->numPages();
    for(int i=0; i < pageCount; i++)
    {
        Poppler::Page *page = document->page(i);

        double vaakaResoluutio = printer->pageRect(QPrinter::Point).width() / page->pageSizeF().width() * printer->resolution();
        double pystyResoluutio = printer->pageRect(QPrinter::Point).height() / page->pageSizeF().height() * printer->resolution();

        double resoluutio = vaakaResoluutio < pystyResoluutio ? vaakaResoluutio : pystyResoluutio;

        page->renderToPainter( &painter, resoluutio, resoluutio,
                                            0,0,page->pageSize().width(), page->pageSize().height());
        if( i < pageCount - 1)
            printer->newPage();

        delete page;
    }
    painter.end();
    delete document;

}
