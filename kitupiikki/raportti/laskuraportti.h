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

#ifndef LASKURAPORTTI_H
#define LASKURAPORTTI_H

#include "raportti.h"
#include "ui_laskuraportti.h"

/**
 * @brief Laskujen raportointi
 */
class LaskuRaportti : public Raportti
{
    Q_OBJECT
public:
    enum PvmRajaus { KaikkiLaskut, RajaaLaskupaiva, RajaaErapaiva};
    enum Lajittelu { Viitenumero, Laskupaiva, Erapaiva, Summa, Asiakas };

    LaskuRaportti();
    ~LaskuRaportti();

    RaportinKirjoittaja raportti() override;

    /**
     * @brief Laskuraportin kirjoittaminen
     *
     * @param saldopvm Mille päivälle avoin saldo lasketaan
     * @param avoimet Tulostetaanko vain ne, joilla saldopäivänä avointa saldoa
     * @param lajittelu Lajitteluperuste
     * @param rajaus Rajausperuste
     * @param mista Rajauksen alkupäivä
     * @param mihin Rajauksen päättymispäivä
     * @return
     */
    static RaportinKirjoittaja kirjoitaRaportti(QDate saldopvm, bool avoimet = true,
                                                Lajittelu lajittelu = Laskupaiva,
                                                PvmRajaus rajaus = KaikkiLaskut, QDate mista = QDate(), QDate mihin = QDate());

protected:

    Ui::Laskuraportti *ui;
};

#endif // LASKURAPORTTI_H
