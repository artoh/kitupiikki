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
#ifndef RAPORTTINAYTIN_H
#define RAPORTTINAYTIN_H

#include "printpreviewnaytin.h"
#include "raportti/raportinkirjoittaja.h"

namespace Naytin {

class RaporttiNaytin : public PrintPreviewNaytin
{
    Q_OBJECT
public:
    RaporttiNaytin(const RaportinKirjoittaja& raportti, QObject *parent = nullptr);

    virtual QString otsikko() const override;

    virtual bool csvMuoto() const override;
    virtual QByteArray csv() const override;

    virtual QByteArray data() const override;

    virtual bool htmlMuoto() const override { return true; }
    virtual QString html() const override;

    virtual bool voikoRaidoittaa() const override { return false;}

public slots:
    virtual void tulosta(QPrinter* printer) const override;

private:
    const RaportinKirjoittaja raportti_;
};

}

#endif // RAPORTTINAYTIN_H
