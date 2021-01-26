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
#include "db/verotyyppimodel.h"

class LaskuAlvCombo : public QComboBox
{
    Q_OBJECT
public:
    enum AsiakasVeroLaji { KAIKKI, EU, KOTIMAA, YKSITYINEN };

    LaskuAlvCombo(QWidget* parent = nullptr, AsiakasVeroLaji asiakasVerolaji = KAIKKI, int alvkoodi = AlvKoodi::EIALV, bool ennakkolasku = false );

    int veroKoodi() const;
    int marginaaliKoodi() const;
    double veroProsentti() const;
};

#endif // LASKUALVCOMBO_H
