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


#include <QDebug>
#include <QFile>
#include <QByteArray>

#include "pdftuonti.h"

#ifdef Q_OS_LINUX
    #include <poppler/qt5/poppler-qt5.h>
#elif defined(Q_OS_WIN)
    #include "poppler-qt5.h"
#endif


PdfTuonti::PdfTuonti()
{

}

bool PdfTuonti::tuoTiedosto(const QString &tiedostonnimi, KirjausWg *wg)
{
    QFile tiedosto(tiedostonnimi);
    tiedosto.open( QIODevice::ReadOnly );

    QByteArray bytes = tiedosto.readAll();
    tiedosto.close();

    Poppler::Document *pdfDoc = Poppler::Document::loadFromData( bytes );

    if( pdfDoc )
    {
        Poppler::Page *pdfsivu = pdfDoc->page(0);
        if( pdfsivu)
        {

            QString ylateksti = pdfsivu->text(QRectF(0, 0, pdfsivu->pageSize().width(), 72 * 5 ));
            if( ylateksti.contains("tiliote", Qt::CaseInsensitive))
                qDebug() << "Tiliote";
            else if( ylateksti.contains("lasku", Qt::CaseInsensitive))
                qDebug() << "Lasku";

        }
        delete pdfsivu;
    }


    delete pdfDoc;

    return true;
}
