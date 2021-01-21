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
#ifndef PAIVAKIRJA_H
#define PAIVAKIRJA_H

#include "raportteri.h"
#include "raportinkirjoittaja.h"
#include "db/tilikausi.h"

class Paivakirja : public Raportteri
{
    Q_OBJECT
public:
    explicit Paivakirja(QObject *parent = nullptr, const QString& kielikoodi = QString());

    void kirjoita(const QDate& mista, const QDate& mihin,
                  int optiot = 0, int kohdennuksella = -1);

    enum { TositeJarjestyksessa = 0b1 ,
           RyhmitteleLajeittain = 0b10 ,
           TulostaKohdennukset  = 0b100 ,
           TulostaSummat        = 0b1000 ,
           Kohdennuksella       = 0b10000 ,           
           AsiakasToimittaja    = 0b1000000 ,
           ErittelePaivat       = 0b10000000

         };

public slots:

private slots:
    void dataSaapuu(QVariant *data);

private:
    int optiot_ = 0;
    Tilikausi oletustilikausi_;
    bool samatilikausi_ = true;

};

#endif // PAIVAKIRJA_H
