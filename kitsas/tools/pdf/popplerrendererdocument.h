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
#include <poppler/qt6/poppler-qt6.h>

class PopplerRendererDocument : public PdfRendererDocument
{
public:
    PopplerRendererDocument(const QByteArray& data);
    ~PopplerRendererDocument();

    virtual int pageCount() override;
    virtual QImage renderPage(int page, double resolution) override;
    virtual QImage renderPageToWidth(int page, double width) override;
    virtual bool locked() const override;

private:
    std::unique_ptr<Poppler::Document> pdfDoc_;


};

#endif // POPPLERRENDERERDOCUMENT_H
