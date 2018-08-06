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

#ifndef RAPORTOIJA_H
#define RAPORTOIJA_H


#include <QDate>
#include <QVector>
#include <QMap>
#include <QObject>

#include "raportinkirjoittaja.h"


/**
 * @brief Muokattavan raportin kirjoittava luokka
 *
 * Tätä luokkaa käytetään kirjoitettaessa muokattavia raportteja.
 *
 * @code
 * Raportoija r("Tuloslaskelma");
 * r.lisaaKausi(QDate(2017,1,1), QDate(2017,31,12));
 * r.lisaaKausi(QDate(2016,1,1), QDate(2016,31,12));
 *
 * Raportinkirjoittaja rk = r.raportti();
 * @endcode
 *
 * Raportti on kertakäyttötuote, eli kun raportti on tulostettu, jää se
 * "sekavaan" tilaan ja seuraavalla tulostuskerralla voi tulostaa vähän mitä sattuu.
 * Eli siis uusi raportti uuteen Raportoijaan!
 *
 */
class Raportoija : public QObject
{       
    Q_OBJECT
public:

    enum RaportinTyyppi
    {
        VIRHEELLINEN = 0,
        TULOSLASKELMA = 1,
        TASE = 2,
        KOHDENNUSLASKELMA = 3
    };

    enum SarakeTyyppi
    {
        TOTEUTUNUT = 0,
        BUDJETTI = 1,
        BUDJETTIERO = 2,
        TOTEUMAPROSENTTI = 3
    };

    /**
     * @brief Alustaa raportoijan muokattavalle raportille
     * @param raportinNimi Asetuksissa oleva raportin nimi
     */
    Raportoija(const QString& raportinNimi);

    /**
     * @brief Lisää raporttikauden (sarakkeen)
     * @param alkaa Alkupäivämäärä
     * @param paattyy Päättymispäivämäärä
     * @param tyyppi Sarakkeen tyyppi (toteutuma, budjetti, ero budjettiin, suhteellinen ero)
     */
    void lisaaKausi(const QDate& alkaa, const QDate &paattyy, int tyyppi = TOTEUTUNUT);
    /**
     * @brief Lisää tasetyyppiseen raporttiin tasepäivän (sarakkeen)
     * @param pvm Tasepäivämäärä (tilikauden viimeinen päivä)
     */
    void lisaaTasepaiva(const QDate& pvm);

    /**
     * @brief Raportin tyyppi
     * @return
     */
    RaportinTyyppi tyyppi() const { return tyyppi_; }

    /**
     * @brief Tulostetaanko raportti kausilta (Tuloslaskelma tai Kohdennuslaskelma)
     *
     * Tälläiselle raportilla valitaan raporttikauden päivämääräväleillä
     *
     * @return
     */
    bool onkoKausiraportti() const  { return tyyppi_ == TULOSLASKELMA || tyyppi_ == KOHDENNUSLASKELMA ; }

    /**
     * @brief Tulostetaanko raportti taseesta
     *
     * Tällaisen raportin kaudet ovat yksittäisiä päiviä, joille kertynyt tase lasketaan
     *
     * @return
     */
    bool onkoTaseraportti() const { return tyyppi_ == TASE;  }

    /**
     * @brief Etsii Kustannuslaskelmaan näissä kausissa käytetyt kohdennukset.
     *
     * Ennen kustannuslaskelmaa pitää joko hakea kohdennukset etsimällä
     * tai lisätä ne itse
     *
     */
    void etsiKohdennukset();

    /**
     * @brief Lisää kohdennuksen laskentaan
     * @param kohdennusId Kohdennuksen id
     */
    void lisaaKohdennus(int kohdennusId);


    /**
     * @brief Kirjoittaa raportin tehdyillä valinnoilla
     * @param tulostaErittelyt Tulostetaanko *-rivien jälkeen tilikohtaiset erittelyt
     * @param csvmuoto Muotoillaanko csv-tulostusta varten
     * @return
     */
    RaportinKirjoittaja raportti(bool tulostaErittelyt = true);

protected:
    enum RivinTyyppi
    {
        OLETUS, SUMMA, OTSIKKO, ERITTELY
    };

    void kirjoitaYlatunnisteet(RaportinKirjoittaja &rk);
    void kirjoitaDatasta(RaportinKirjoittaja &rk, bool tulostaErittelyt);

    /**
     * @brief Sijoittaa tulostilien kyselyn dataan
     * @param kysymys Sql-kysely tekstinä
     */
    void sijoitaTulosKyselyData(const QString& kysymys, int i);

    void laskeTulosData();
    void laskeTaseDate();

    /**
     * @brief Laskee kohdennusten datan
     * @param kohdennusId Kohdennuksen id
     * @param poiminnassa tosi, jos tulostetaan tasemuodossa
     */
    void laskeKohdennusData(int kohdennusId, bool poiminnassa=false);

    QString sarakeTyyppiTeksti(int sarake);

    void sijoitaBudjetti(int kohdennus = -1);



protected:
    QString otsikko_;
    QStringList kaava_;
    QString optiorivi_;

    RaportinTyyppi tyyppi_;

    QVector<QDate> alkuPaivat_;
    QVector<QDate> loppuPaivat_;
    QVector<int> sarakeTyypit_;

    QVector< QMap< int, qlonglong> > data_;    // ysiluku, sentit
    QVector< QMap< int, qlonglong> > budjetti_; // ysiluku, sentit
    QMap<int,bool> tilitKaytossa_;           // ysiluku
    std::list<int> kohdennusKaytossa_;       // kohdennusId


};

#endif // RAPORTOIJA_H
