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
    enum Linkki
    {
        EI_LINKKIA = 0,
        TILI_NRO = 1,
        TOSITE_ID = 2,
        TILI_LINKKI = 3
    };

    bool tasaaOikealle = false;

    QString teksti;
    int leveysSaraketta = 1;

    Linkki linkkityyppi = EI_LINKKIA;
    int linkkidata = 0;
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
     * @brief Lisää tekstisarakkeen, joka toimii linkkinä
     *
     * Linkillä sähköisessä arkistossa raportista päästään porautumaan tietoihin
     *
     * @param linkkityyppi Linkin tyyppi (TILI_NUMERO tai TOSITE_ID)
     * @param linkkitieto Linkitettävä tieto
     * @param teksti Tulostuva teksti
     * @param sarakkeet Leveys saraketta
     */
    void lisaaLinkilla(RaporttiRiviSarake::Linkki linkkityyppi, int linkkitieto,
                       const QString& teksti, int sarakkeet = 1);
    /**
     * @brief Lisää rahamäärän
     * @param sentit Rahamäärä sentteinä
     * @param tulostanollat Tulostetaanko nollat (oletuksena ei)
     */
    void lisaa(qlonglong sentit,bool tulostanollat = false);

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
     * @brief Palauttaa sarakkeen
     * @param indeksi Sarakkeen indeksi
     * @return
     */
    RaporttiRiviSarake sarake(int indeksi) { return sarakkeet_[indeksi]; }

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

    /**
     * @brief Lihavoi tämän rivin
     * @param lihavaksi
     */
    void lihavoi(bool lihavaksi=true) { lihava_ = lihavaksi; }

    /**
     * @brief Onko tämä rivi lihavoitu
     * @return
     */
    bool onkoLihava() const { return lihava_; }

    /**
     * @brief Piirretään rivin ylle viiva
     * @param viivaksi
     */
    void viivaYlle(bool viivaksi=true) { ylaviiva_ = viivaksi; }

    /**
     * @brief Piirretäänkö tämän rivin yläpuolelle viiva
     * @return
     */
    bool onkoViivaa() const { return ylaviiva_; }

    /**
     * @brief Asettaa käytettävän fonttikoon
     * @param pistekoko
     */
    void asetaKoko(int pistekoko) { pistekoko_ = pistekoko; }

    /**
     * @brief Palauttaa käytettävän pistekoon
     * @return
     */
    int pistekoko() const { return pistekoko_; }


protected:
    QList<RaporttiRiviSarake> sarakkeet_;
    bool lihava_;
    bool ylaviiva_;
    int pistekoko_;
};

#endif // RAPORTTIRIVI_H
