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
#include "poppleranalyzerdocument.h"
#include "pdfanalyzerpage.h"

PopplerAnalyzerDocument::PopplerAnalyzerDocument(const QByteArray &data)
{
    pdfDoc_ = Poppler::Document::loadFromData(data);
}

PopplerAnalyzerDocument::~PopplerAnalyzerDocument()
{
    if( pdfDoc_)
        delete pdfDoc_;
}

int PopplerAnalyzerDocument::pageCount()
{
    if(pdfDoc_ && !pdfDoc_->isLocked())
        return pdfDoc_->numPages();
    else
        return 0;
}

PdfAnalyzerPage PopplerAnalyzerDocument::page(int page)
{
    PdfAnalyzerPage result;

    if( pdfDoc_ && !pdfDoc_->isLocked()) {

        Poppler::Page *sivu = pdfDoc_->page(page);
        if( sivu) {
            result.setSize( sivu->pageSizeF() );

            auto lista = sivu->textList();
            for(int i=0; i < lista.count(); i++) {
                QRectF rect = lista.at(i)->boundingBox();
                QString txt = lista.at(i)->text();
                if( lista.at(i)->hasSpaceAfter() )
                    txt.append(" ");

                while(lista.at(i)->nextWord()) {
                    i++;
                    rect = rect.united( lista.at(i)->boundingBox() );
                    txt.append(lista.at(i)->text());
                    if( lista.at(i)->hasSpaceAfter() )
                        txt.append(" ");
                }
                result.addText( rect, txt);

            }
            delete sivu;
        }
    }

    return result;
}

QString PopplerAnalyzerDocument::title() const
{
    if( pdfDoc_)
        return pdfDoc_->title();
    else
        return QString();
}
