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
#ifndef PDFVIEW_H
#define PDFVIEW_H

#include "abstraktiview.h"


namespace Naytin {

class PdfView : public AbstraktiView
{
    Q_OBJECT
public:
    PdfView(const QByteArray& pdf);

    virtual QString tiedostonMuoto() const override { return tr("pdf-tiedosto (*.pdf)");}
    virtual QString tiedostonPaate() const override { return "pdf"; }

    virtual QByteArray data() const override;

    virtual QString otsikko() const override;

public slots:
    void paivita() const override;
    void tulosta(QPrinter* printer) const override;

    virtual void zoomIn() override;
    virtual void zoomOut() override;
    virtual void zoomFit() override;

protected:
    QByteArray data_;    
    qreal skaala_;

};


}


#endif // PDFVIEW_H
