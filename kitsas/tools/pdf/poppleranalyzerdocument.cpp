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

#include <iostream>

#include <QMap>

PopplerAnalyzerDocument::PopplerAnalyzerDocument(const QByteArray &data)
{
    pdfDoc_ = Poppler::Document::loadFromData(data);
}

PopplerAnalyzerDocument::~PopplerAnalyzerDocument()
{
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    delete pdfDoc_;
#endif
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

           auto sivu = pdfDoc_->page(page);

           if(sivu) {
               result.setSize(sivu->pageSizeF());
               QMap<int,PdfAnalyzerRow> rows;
               PdfAnalyzerText text;

               auto lista = sivu->textList();
               for(const auto& item : lista) {
                   text.addWord(item->boundingBox(), item->text(), item->hasSpaceAfter());
                   if( !item->hasSpaceAfter()) {
                       int indeksi = qRound( text.boundingRect().top() );
                       if( rows.contains(indeksi-1) )
                           indeksi = indeksi -1;
                       else if( rows.contains(indeksi+1))
                           indeksi = indeksi + 1;
                       if( text.boundingRect().right() > 25)
                           rows[indeksi].addText(text);
                       text = PdfAnalyzerText();
                   }
               }
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
               delete sivu;
#endif

               QMapIterator<int,PdfAnalyzerRow> iter(rows);
               while(iter.hasNext()) {
                   iter.next();
                   result.addRow(iter.value());
               }
           }
       }
    return result;
}



QList<PdfAnalyzerPage> PopplerAnalyzerDocument::allPages()
{
    QList<PdfAnalyzerPage> pages;
    for(int i=0; i < pageCount(); i++)
        pages.append( page(i) );
    return pages;
}

QString PopplerAnalyzerDocument::title() const
{
    if( pdfDoc_)
        return pdfDoc_->title();
    else
        return QString();
}
