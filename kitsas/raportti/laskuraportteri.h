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
#ifndef LASKURAPORTTERI_H
#define LASKURAPORTTERI_H

#include "raportteri.h"
#include "raportinkirjoittaja.h"

class LaskuRaportteri : public Raportteri
{
    Q_OBJECT
public:
    LaskuRaportteri(QObject *parent = nullptr, const QString kielikoodi = QString());

    enum {  Myyntilaskut        = 0b1,
            Ostolaskut          = 0b10,
            RajaaLaskupaivalla  = 0b100,
            RajaaErapaivalla    = 0b1000,
            VainAvoimet         = 0b10000,
            LajitteleNumero     = 0b100000,
            LajitteleViite      = 0b1000000,
            LajittelePvm        = 0b10000000,
            LajitteleErapvm     = 0b100000000,
            LajitteleSumma      = 0b1000000000,
            LajitteleAsiakas    = 0b10000000000,
            NaytaViite          = 0b100000000000,
            TulostaSummat       = 0b1000000000000,
            VainKitsas          = 0b10000000000000
         };

    void kirjoita(int optiot, const QDate& saldopvm,
                  const QDate& mista = QDate(), const QDate& mihin = QDate());

public:
    bool lajitteluVertailu(const QVariant& eka, const QVariant& toka);

private:
    void dataSaapuu(QVariant* data);

    int optiot_ = 0;
    QDate saldopvm_;

};

#endif // LASKURAPORTTERI_H
