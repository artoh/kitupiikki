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
#ifndef PDFSCENE_H
#define PDFSCENE_H

#include "naytinscene.h"
#include <QByteArray>

class PdfScene : public NaytinScene
{
    Q_OBJECT
public:
    PdfScene(QObject *parent = nullptr);
    PdfScene(const QByteArray& pdfdata, QObject *parent = nullptr);

    QString otsikko() const override;

    bool naytaPdf(const QByteArray& pdfdata);

    QString tyyppi() const override { return "pdf"; }

    void piirraLeveyteen(double leveyteen) override;

protected:
    QByteArray data_;
    QString otsikko_;
};

#endif // PDFSCENE_H
