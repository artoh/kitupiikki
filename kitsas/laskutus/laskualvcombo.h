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
#ifndef LASKUALVCOMBO_H
#define LASKUALVCOMBO_H

#include <QComboBox>
#include <QDate>
#include "db/verotyyppimodel.h"

// TÄMÄ PITÄNEE VIELÄ UUDISTAA VOI EI SENTÄÄN !!!!

class LaskuAlvCombo : public QComboBox
{
    Q_OBJECT
public:
    enum AsiakasVeroLaji { KAIKKI, EU, KOTIMAA, YKSITYINEN };

    LaskuAlvCombo(QWidget* parent = nullptr);

    void alusta(AsiakasVeroLaji asiakasVerolaji = KAIKKI, int alvkoodi = AlvKoodi::EIALV,
                bool ennakkolasku = false, const QDate& pvm = QDate());

    void aseta(int alvkoodi, double alvprosentti = 0.0);

    int veroKoodi() const;
    double veroProsentti() const;
};

#endif // LASKUALVCOMBO_H
