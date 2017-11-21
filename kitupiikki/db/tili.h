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

#ifndef TILI_H
#define TILI_H

#include <QString>
#include <QDate>

#include "jsonkentta.h"
#include "tilityyppimodel.h"

/**
 * @brief Tilin tai otsikon tiedot
 *
 */
class Tili
{
public:
    Tili();
    Tili(int id, int numero, const QString& nimi, const QString& tyyppiKoodi, int tila,
         int ylaotsikkoid = 0);

    int id() const { return id_; }
    int numero() const { return numero_; }
    QString nimi() const { return nimi_; }
    TiliTyyppi tyyppi() const { return tyyppi_;}
    QString tyyppiKoodi() const { return tyyppi().koodi(); };
    int tila() const { return tila_; }
    int otsikkotaso() const { return tyyppi().otsikkotaso(); }
    bool muokattu() const { return muokattu_ || json_.muokattu(); }

    /**
     * @brief Palauttaa tämän tilin tai otsikon yllä olevan otsikon id:n
     * @return
     */
    int ylaotsikkoId() const { return ylaotsikkoId_; }
    /**
     * @brief Palauttaa json-tiedot
     *
     * JsonKentta-oliossa on mahdollisuus säilöä erilaista tietoa, jonka lisääminen
     * ei edellytä tietokannan muutoksia.
     *
     * @return
     */
    JsonKentta *json()  { return &json_; }

    void asetaId(int id) { id_ = id; }
    void asetaNumero(int numero) { numero_ = numero; muokattu_ = true; }
    void asetaNimi(const QString& nimi) { nimi_ = nimi; muokattu_ = true; }
    void asetaTyyppi(const QString& tyyppikoodi);
    void asetaTila(int tila) { tila_ = tila; muokattu_ = true; }

    void nollaaMuokattu() { muokattu_ = false; }

    /**
     * @brief Onko tilillä tarvittavat tiedot, että voi tallettaa
     * @return
     */
    bool onkoValidi() const;

    /**
     * @brief Yhdeksännumeroinen tilinumeron vertailuluku, jonka avulla
     * tilit saadaan oikeaan järjestykseen.
     *
     * Viimeinen numero kertoo otsikkotason - 1, tileillä se on 9
     * Otsikkotasoja saa olla siis enintään yhdeksän
     *
     * tili 1234 -> 123400009
     * 2. tason otsikko 1230 -> 123000001
     *
     * @return
     */
    int ysivertailuluku() const;


    /**
     * @brief Laskee tilin saldon päivämäärälle
     *
     * Tasetilin saldo lasketaan kirjanpidon alusta ja tulostilin saldo
     * tilikauden alusta
     *
     * @param pvm Päivämäärä, jolle saldo lasketaan
     * @return Saldo sentteinä
     */
    int saldoPaivalle(const QDate &pvm);

    /**
     * @brief Montako kirjausta tälle tilille
     *
     * Käytetään valvomaan sitä, ettei käytössä olevaa tiliä voi poistaa
     *
     * @return Kirjausten määrä
     */
    int montakoVientia() const;

    /**
     * @brief Onko tili kysyttyä tyyppiä
     * @param luonne Tilin luonne
     * @return
     *
     * Luonne voi olla varsinainen tilityyppi tai sen yleistys, esimerkiksi
     * TiliLaji::Tulos kuvaa kaikkia tulo- ja menotilejä.
     *
     */
    bool onko(TiliLaji::TiliLuonne luonne) const;


    enum TaseErittelyTapa
    {
        TASEERITTELY_EI  = 0,
        TASEERITTELY_SALDOT = 1,
        TASEERITTELY_MUUTOKSET = 2,
        TASEERITTELY_TAYSI = 3
    };

    /**
     * @brief Millä tasolla tase-erittely laaditaan
     * @return TaseErittelyTapa
     */
    int taseErittelyTapa() { return json()->luku("Taseerittely"); }

    /**
     * @brief Laskee yhdeksännumeroisen vertailuluvun
     *
     * Näiden vertailulukujen avulla tilit saadaan järjestykseen ja voidaan vertailla, kuuluuko
     * tili jollekin välille. Ysiluku saadaan pidentämällä numero yhdeksännumeroiseksi niin,
     * että 1234 -> 123400000 mutta lopussa 1234 -> 123499999
     *
     * @param luku Pidennettävä tililuku
     * @param loppuu Tosi, jos halutaan loppukasiluku
     * @return Yhdeksännnumeroiseksi pidennetty tilinumero
     */
    static int ysiluku(int luku, bool loppuu = false);
    /**
     * @brief Laskee yhdeksännumeroisen vertailuluvun
     *
     * Pidennyksen viimeinen numero on tileillä 9, otsikoilla (taso-1)
     * Näin tätä lukua voidaan käyttää tilien lajitteluun
     *
     * @param luku Pidennettävä tililuku
     * @param taso Tasonumero
     * @return
     */
    static int ysiluku(int luku, int taso);

protected:
    static int laskeysiluku(int luku, bool loppuu = false);

protected:
    int id_;
    int numero_;
    QString nimi_;
    TiliTyyppi tyyppi_;
    int tila_;
    JsonKentta json_;
    int ylaotsikkoId_;
    bool muokattu_;

};

#endif // TILI_H
