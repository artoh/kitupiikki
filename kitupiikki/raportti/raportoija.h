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

#include "raportteri.h"
#include "raportinkirjoittaja.h"


/**
 * @brief Muokattavan raportin kirjoittava luokka
 *
 * Tätä luokkaa käytetään kirjoitettaessa muokattavia raportteja.
 *
 * @code
 * Raportoija r("tase/PMA");
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
class Raportoija : public Raportteri
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
    Raportoija(const QString& raportinNimi,
               const QString& kieli = "fi",
               QObject* parent = nullptr);
    ~Raportoija() override;

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

    void kirjoita(bool tulostaErittelyt = false);

protected:
    void dataSaapuu(int sarake, QVariant* variant);
    void dataSaapunut();


protected:
    enum RivinTyyppi
    {
        OLETUS, SUMMA, OTSIKKO, ERITTELY
    };

    void kirjoitaYlatunnisteet();
    void kirjoitaDatasta();

    QString sarakeTyyppiTeksti(int sarake);



protected:
    QVariantMap kmap_;
    QString kieli_;
    bool erittelyt_;

    RaportinTyyppi tyyppi_;

    QVector<QDate> alkuPaivat_;
    QVector<QDate> loppuPaivat_;
    QVector<int> sarakeTyypit_;

    QHash<int, QVector<qlonglong> > snt_;   // tili -> sentit  (tot,tot,tot,bud,bud,bud)
    QStringList tilit_;

    int tilausLaskuri_ = 0;
    int sarakemaara_ = 0;

};

#endif // RAPORTOIJA_H
