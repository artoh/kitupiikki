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
#ifndef KUVANAYTIN_H
#define KUVANAYTIN_H

#include "naytinscene.h"

#include <QImage>

class KuvaNaytin : public NaytinScene
{
    Q_OBJECT
public:
    KuvaNaytin(QObject *parent = nullptr);
    KuvaNaytin(const QByteArray& kuvadata, QObject *parent = nullptr);

    bool naytaKuva(const QByteArray& kuvadata);

    QString tyyppi() const override;
    QString otsikko() const override { return tr("Kitupiikki");}

    void piirraLeveyteen(double leveyteen) override;

    QString tiedostonMuoto() override { return tr("jpg-kuvatiedosto (*.jpg)");}
    QString tiedostoPaate() override { return "jpg"; }
    QByteArray data() override;

    void tulosta(QPrinter* printer) override;

private:
    QImage kuva_;
};

#endif // KUVANAYTIN_H
