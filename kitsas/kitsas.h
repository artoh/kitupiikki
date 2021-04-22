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
#ifndef KITSAS_H
#define KITSAS_H

namespace AlvKoodi {
    /**
     * @brief Viennin alv-kirjauksen laji
     */
    enum Koodi
    {
        EIALV = 0,
        ALV0 = 19,
        MYYNNIT_NETTO = 11,
        OSTOT_NETTO = 21,
        MYYNNIT_BRUTTO = 12,
        OSTOT_BRUTTO = 22,
        MYYNNIT_MARGINAALI = 13,
        OSTOT_MARGINAALI = 23,
        YHTEISOMYYNTI_TAVARAT = 14,
        YHTEISOHANKINNAT_TAVARAT = 24,
        YHTEISOMYYNTI_PALVELUT = 15,
        YHTEISOHANKINNAT_PALVELUT = 25,
        RAKENNUSPALVELU_MYYNTI = 16,
        RAKENNUSPALVELU_OSTO = 26,
        MAAHANTUONTI = 27,
        MAAHANTUONTI_PALVELUT = 29,
        MAKSUPERUSTEINEN_MYYNTI = 18,
        MAKSUPERUSTEINEN_OSTO = 28,
        ENNAKKOLASKU_MYYNTI = 51,
        ALVKIRJAUS = 100,
        ALVVAHENNYS = 200,
        MAKSUPERUSTEINEN_KOHDENTAMATON = 400,
        MAKSETTAVAALV = 900,
        TILITYS = 901,
        MAAHANTUONTI_VERO = 927,
        VAHENNYSKELVOTON = 932
    };

};

namespace Kitsas {
    enum {
        UUSI_ERA = -1
    };
}

#endif // KITSAS_H
