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
#ifndef POPPLERRENDERERDOCUMENT_H
#define POPPLERRENDERERDOCUMENT_H

#include "pdftoolkit.h"
#include <poppler/qt5/poppler-qt5.h>

class PopplerRendererDocument : public PdfRendererDocument
{
public:
    PopplerRendererDocument(const QByteArray& data);
    ~PopplerRendererDocument();

    virtual int pageCount() override;
    virtual QImage page(int page, double xres, double yres) override;
    virtual QImage renderPageToWidth(int page, double width) override;
    virtual bool locked() const override;

private:
    Poppler::Document *pdfDoc_ = nullptr;


};

#endif // POPPLERRENDERERDOCUMENT_H
