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

#include <QBuffer>

PopplerRendererDocument::PopplerRendererDocument(const QByteArray &data)
{

}

PopplerRendererDocument::~PopplerRendererDocument()
{
}

int PopplerRendererDocument::pageCount()
{
    return 0;
}

QImage PopplerRendererDocument::renderPage(int /* page */, double /* resolution */)
{    

    return QImage();
}

bool PopplerRendererDocument::locked() const
{
    return true;
}


QImage PopplerRendererDocument::renderPageToWidth(int /* page */, double /* width */)
{
    return QImage();
}
