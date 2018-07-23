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
#ifndef RAPORTTISCENE_H
#define RAPORTTISCENE_H

#include "pdfscene.h"
#include "raportti/raportinkirjoittaja.h"


class RaporttiScene : public PdfScene
{
    Q_OBJECT
public:
    RaporttiScene(QObject *parent = nullptr);
    RaporttiScene(RaportinKirjoittaja raportti, QObject *parent = nullptr);

    void nayta(RaportinKirjoittaja raportti);

    QString tyyppi() const override { return "raportti"; };

    QString otsikko() const override;
    virtual bool csvMuoto() override;

protected:
    virtual QByteArray csv() override;


private:
    RaportinKirjoittaja raportti_;
};

#endif // RAPORTTISCENE_H
