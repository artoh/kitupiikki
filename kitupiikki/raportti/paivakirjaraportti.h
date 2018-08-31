/*
   Copyright (C) 2017 Arto Hyvättinen

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

#ifndef PAIVAKIRJARAPORTTI_H
#define PAIVAKIRJARAPORTTI_H

#include "raportti.h"
#include "ui_paivakirja.h"

/**
 * @brief Päiväkirjan tulostava raportti
 *
 * Päiväkirjassa on halutun päivämäärävälin viennit aikajärjestyksessä
 */
class PaivakirjaRaportti : public Raportti
{
    Q_OBJECT
public:
    PaivakirjaRaportti();
    ~PaivakirjaRaportti();

    RaportinKirjoittaja raportti();


    /**
     * @brief Päiväkirjaraportin kirjoittaminen
     *
     * Staattinen funktio, jotta raportti voidaan kirjoittaa myös ilman käyttöliittymää
     *
     * @param kohdennuksella Tulostaa vain yhden kohdennuksen osalta: kohdennuksen Id, -1 jos kaikki
     * @param tositejarjestys Tulostaa tositejärjestyksessä, muuten päivämääräjärjestyksessä
     * @param ryhmitalajeittain Ryhmittelee toisitelajeittain
     * @param tulostakohdennukset Tulostaa kohdennussarakkeen
     * @param tulostasummat Tulostaa summarivit
     * @return Raportinkirjoittaja, jonne raportti kirjoitettu
     */
    static RaportinKirjoittaja kirjoitaRaportti( QDate mista, QDate mihin,
                                 int kohdennuksella = -1, bool tositejarjestys = false,
                                 bool ryhmitalajeittain = false, bool tulostakohdennukset = false,
                                 bool tulostasummat = false);

protected:
    static void kirjoitaSummaRivi(RaportinKirjoittaja &rk, qlonglong debet, qlonglong kredit, int sarakeleveys);

    Ui::Paivakirja *ui;
};

#endif // PAIVAKIRJARAPORTTI_H
