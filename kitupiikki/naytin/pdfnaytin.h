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
#ifndef PDFNAYTIN_H
#define PDFNAYTIN_H

#include "printpreviewnaytin.h"

namespace Naytin {

class PdfNaytin : public PrintPreviewNaytin
{
public:
    PdfNaytin(const QByteArray& pdfdata, QObject *parent = nullptr);

    virtual QString otsikko() const override;

    virtual QByteArray data() const override;

public slots:
    virtual void tulosta(QPrinter *printer) const override;

protected:
    QByteArray data_;
};


}



#endif // PDFNAYTIN_H
