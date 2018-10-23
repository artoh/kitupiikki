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

#ifndef ALVERITTELY_H
#define ALVERITTELY_H

// Toistaiseksi käytetään tase-erittelyn ui:ta, koska sopii myös tähän

#include "raportti.h"

namespace Ui {
    class TaseErittely;
}


/**
 * @brief Arvonlisävero-erittelyn tulostaminen
 */
class AlvErittely : public Raportti
{
    Q_OBJECT
public:
    AlvErittely();
    ~AlvErittely();

    RaportinKirjoittaja raportti() override;

    static RaportinKirjoittaja kirjoitaRaporti(QDate alkupvm, QDate loppupvm);

protected:
    Ui::TaseErittely *ui;

};

#endif // ALVERITTELY_H
