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
#include "qregularexpression.h"

#include <iostream>

#include <QMap>
#include <QBuffer>

#include "tuonti/pdf/pdfsivu.h"

PopplerAnalyzerDocument::PopplerAnalyzerDocument(const QByteArray &data)
{
    QByteArray arr(data);
    QBuffer buff(&arr);
    buff.open(QIODevice::ReadOnly);

    connect( &doc_, &QPdfDocument::statusChanged, this, &PopplerAnalyzerDocument::status);
    doc_.load(&buff);
}

PopplerAnalyzerDocument::~PopplerAnalyzerDocument()
{

}

int PopplerAnalyzerDocument::pageCount()
{
    return doc_.pageCount();
}


PdfAnalyzerPage PopplerAnalyzerDocument::page(int page)
{   

    PdfAnalyzerPage result;
//    QPdfSelection selection = doc_.getAllText(page);

//    result.setSize( doc_.pagePointSize(page) );

    return result;



/*
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

*/
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
    return doc_.metaData(QPdfDocument::MetaDataField::Title).toString();
}

void PopplerAnalyzerDocument::status(QPdfDocument::Status s) {
    qDebug() << "Status " << s;



    if( s == QPdfDocument::Status::Ready) {

        Tuonti::PdfSivu sivu;
        sivu.tuo(&doc_, 0);

        std::cerr << sivu.teksti().toStdString();

/*
        QRegularExpression empty("\\s");

        QPdfSelection sel = doc_.getAllText(0);
        qDebug() << sel.text();

        QMap<int,Palanen*> palat;

        QString txt = sel.text();
        int c = 0;
        while( c < txt.length()) {
            int stop = txt.indexOf(empty, c);
            if( stop < 0) stop = txt.length()-1;
            if( stop == c) { c++; continue; }

            QPdfSelection piece = doc_.getSelectionAtIndex(0, c, stop - c);

            int sijainti = (int) piece.boundingRectangle().top() * 1000 + (int) piece.boundingRectangle().left();
            Palanen* pala = new Palanen();
            pala->ala = piece.boundingRectangle().bottom();
            pala->yla = piece.boundingRectangle().top();
            pala->oikea = piece.boundingRectangle().right();
            pala->vasen = piece.boundingRectangle().left();
            pala->teksti = piece.text().trimmed();

            palat.insert(sijainti, pala);

            std::cerr << pala->yla << "," << pala->vasen << "|" << pala->teksti.toStdString() << "| " << pala->ala << "," << pala->oikea << "\n";

            c = stop + 1;
        }

        Palanen* eka = nullptr;
        Palanen* edellinen = nullptr;

        QMapIterator<int,Palanen*> iter(palat);
        while(iter.hasNext()) {
            iter.next();
            if( !eka) { eka = iter.value(); edellinen = iter.value(); std::cerr << "***ALKU \n";}
            else {
                if( qAbs(edellinen->yla - iter.value()->yla) < 8 &&
                   qAbs(edellinen->oikea - iter.value()->vasen) <= 14 ) {
                   edellinen->oikea = iter.value()->oikea;
                   edellinen->teksti.append(" " + iter.value()->teksti);
                   std::cerr << "+ " << iter.value()->teksti.toStdString() << "}\n";
                } else {
                   edellinen->seuraava = iter.value();
                   edellinen = iter.value();
                   std::cerr << "> " << iter.value()->teksti.toStdString() << "}\n";
                }
            }
        }
        while( eka != nullptr) {
            std::cerr << eka->yla << "," << eka->vasen << "  |" << eka->teksti.toStdString() << "| " << eka->oikea << "\n";
            eka = eka->seuraava;
        }
*/
    }

}

PopplerAnalyzerDocument::Palanen::Palanen()
{

}
