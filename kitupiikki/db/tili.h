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

/**
 * @brief Tilin tai otsikon tiedot
 *
 * json-määreitä
 * -------------
 * Vastatili:  Oletusvastatili kirjauksille
 * AlvProsentti: Oletus arvonlisäveron prosentti
 **
 */
class Tili
{
public:
    Tili();
    Tili(int id,int numero, const QString& nimi, const QString& tyyppi, int tila, int otsikkotaso = 0);

    int id() const { return id_; }
    int numero() const { return numero_; }
    QString nimi() const { return nimi_; }
    QString tyyppi() const { return tyyppi_; }
    int tila() const { return tila_; }
    int otsikkotaso() const { return otsikkotaso_; }
    bool muokattu() const { return muokattu_ || json_.muokattu(); }

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
    void asetaTyyppi(const QString& tyyppi) { tyyppi_ = tyyppi, muokattu_ = true; }
    void asetaTila(int tila) { tila_ = tila; muokattu_ = true; }
    void asetaOtsikkotaso(int taso) { otsikkotaso_ = taso; muokattu_ = true; }

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
     * @brief Laskee alusta ko. paivan loppuun saakka kertyneen saldon
     *
     * Tasetileillä tämä on saldo ko. päivän lopussa,
     * tulostileillä tästä pitää vähentää kertymä edellisen tilikauden
     * lopussa
     *
     * @param pvm Päivä, jonka loppuun kertymä lasketaan
     * @return Kertymä sentteinä
     */
    int kertymaPaivalle(const QDate &pvm);

    /**
     * @brief Montako kirjausta tälle tilille
     *
     * Käytetään valvomaan sitä, ettei käytössä olevaa tiliä voi poistaa
     *
     * @return Kirjausten määrä
     */
    int montakoVientia() const;

    bool onkoTasetili() const;
    bool onkoTulotili() const;
    bool onkoMenotili() const;
    bool onkoVastaavaaTili() const;
    bool onkoVastattavaaTili() const;

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

protected:
    static int laskeysiluku(int luku);

protected:
    int id_;
    int numero_;
    QString nimi_;
    QString tyyppi_;
    int tila_;
    int otsikkotaso_;
    JsonKentta json_;
    bool muokattu_;
};

#endif // TILI_H
