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

#ifndef RAPORTTIRIVI_H
#define RAPORTTIRIVI_H

#include <QString>
#include <QDate>
#include <QList>

/**
 * @brief Yhden raportin sarakkeen tiedot, RaporttiRivin käyttöön
 */
struct RaporttiRiviSarake
{
    bool tasaaOikealle = false;

    QString teksti;
    int leveysSaraketta = 1;
};

/**
 * @brief Yksi raportin tai otsakkeen rivi
 *
 * Käytetään kirjoitettaessa raportteja RaportinKirjoittaja:lla
 *
 * @see RaportinKirjoittaja
 *
 */
class RaporttiRivi
{
public:
    RaporttiRivi();

    /**
     * @brief Lisää tekstisarakkeen
     * @param teksti Lisättävä teksti
     * @param sarakkeet Kuinka monen sarakkeen levyisenä
     * @param tasaaOikealle Tasataanko oikealle (oletuksena vasemmalle)
     */
    void lisaa(const QString& teksti, int sarakkeet = 1, bool tasaaOikealle = false);
    /**
     * @brief Lisää rahamäärän
     * @param sentit Rahamäärä sentteinä
     * @param tulostanollat Tulostetaanko nollat (oletuksena ei)
     */
    void lisaa(int sentit,bool tulostanollat = false);

    /**
     * @brief Lisää päivämäärän
     * @param pvm Päivämäärä QDate:na
     */
    void lisaa(const QDate& pvm);

    /**
     * @brief Sarakkeiden lukumäärä
     * @return
     */
    int sarakkeita() const { return sarakkeet_.count(); }

    /**
     * @brief Tulostettava teksti
     * @param sarake Sarakkeen indeksi
     * @return
     */
    QString teksti(int sarake) { return sarakkeet_[sarake].teksti; }

    /**
     * @brief Kuinka monta ruudukkosaraketta tämä sarake täyttää
     * @param sarake
     * @return
     */
    int leveysSaraketta(int sarake) { return sarakkeet_[sarake].leveysSaraketta; }

    /**
     * @brief Onko sarake tasattu oikealle
     * @param sarake
     * @return
     */
    bool tasattuOikealle(int sarake) { return sarakkeet_[sarake].tasaaOikealle; }

    /**
     * @brief Tyhjentää otsikkorivin
     */
    void tyhjenna() { sarakkeet_.clear(); }

protected:
    QList<RaporttiRiviSarake> sarakkeet_;
};

#endif // RAPORTTIRIVI_H
