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
#include "popplerrendererdocument.h"



PopplerRendererDocument::PopplerRendererDocument(const QByteArray &data)
{
    pdfDoc_ = Poppler::Document::loadFromData(data);

    if( pdfDoc_) {
        pdfDoc_->setRenderHint(Poppler::Document::TextAntialiasing);
        pdfDoc_->setRenderHint(Poppler::Document::Antialiasing);
    }
}

PopplerRendererDocument::~PopplerRendererDocument()
{
    if(pdfDoc_)
        delete pdfDoc_;
}

int PopplerRendererDocument::pageCount()
{
    if(pdfDoc_)
        return pdfDoc_->numPages();
    else
        return 0;
}

QImage PopplerRendererDocument::renderPage(int page, double resolution)
{
    if( !pdfDoc_ || locked())
        return QImage();

    Poppler::Page *pdfSivu = pdfDoc_->page(page);
    if( !pdfSivu)
        return QImage();

    QImage image = pdfSivu->renderToImage(resolution, resolution);
    delete pdfSivu;
    return image;
}

bool PopplerRendererDocument::locked() const
{
    if( pdfDoc_)
        return pdfDoc_->isLocked();
    else
        return false;
}


QImage PopplerRendererDocument::renderPageToWidth(int page, double width)
{
    if( !pdfDoc_ || locked())
        return QImage();

    Poppler::Page *pdfSivu = pdfDoc_->page(page);
    if( !pdfSivu)
        return QImage();

    double pdfleveys = pdfSivu->pageSizeF().width();
    double skaala = width / pdfleveys * 72.0;

    QImage image = pdfSivu->renderToImage(skaala, skaala);
    delete pdfSivu;
    return image;
}
