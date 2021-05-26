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
#include "esikatseltava.h"

#include "naytinikkuna.h"
#include "naytinview.h"

#include <QPrinter>
#include <QBuffer>
#include <QPdfWriter>
#include <QApplication>


Esikatseltava::Esikatseltava()
{

}

Esikatseltava::~Esikatseltava()
{
}

void Esikatseltava::esikatsele()
{

    NaytinIkkuna *ikkuna = new NaytinIkkuna();
    ikkuna->show();

    ikkuna->view()->esikatsele(this);    
}

QByteArray Esikatseltava::pdf() const
{
    QByteArray array;
    QBuffer buffer(&array);
    buffer.open(QIODevice::WriteOnly);

    QPdfWriter writer(&buffer);
    writer.setPdfVersion(QPagedPaintDevice::PdfVersion_A1b);
    writer.setCreator(QString("%1 %2").arg( qApp->applicationName() , qApp->applicationVersion() ));
    writer.setTitle( otsikko() );
    writer.setResolution(1200);

    tulosta( &writer );

    buffer.close();

    return array;
}
