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

#ifndef RAPORTINKIRJOITTAJA_H
#define RAPORTINKIRJOITTAJA_H

#include <QString>
#include <QList>
#include <QPrinter>

#include "raporttirivi.h"

/**
 * @brief  Yksi raportin sarake, RaportinKirjoittajan sisäiseen käyttöön
 */
struct RaporttiSarake
{
    QString leveysteksti;
    int jakotekija = 0;
    int leveysprossa = 0;

};

/**
 * @brief Raporttien kirjoittaja
 *
 * Tällä luokalla kirjoitetaan raportit
 *
 * Raportin ylätunniste tulostetaan, jos sille on määritelty otsikko
 *
 * @code
 *    Raportinkirjoittaja kirjoittaja;
 *    kirjoittaja.asetaOtsikko("RAPORTTI");
 * @endcode
 *
 * Raportille voi lisätä yhden tai useamman otsakerivin, jotka toistuvat joka sivulla
 *
 * @code
 *    RaporttiRivi otsikko;
 *    otsikko.lisaa("Pvm");
 *    otsikko.lisaa("Selite");
 *    otsikko.lisaa("Euroa", 1, true);  // Yhden sarakkeen leveys, tasaus oikealle
 *    kirjoittaja.lisaaOtsake(otsikko);
 * @endcode
 *
 * Sitten raportille lisätään sisältö
 *
 * @code
 *    RaporttiRivi rivi;
 *    rivi.lisaa( QDate(2017,12,31) );
 *    rivi.lisaa( "Oma selite" );
 *    rivi.lisaa( 1240 );  // 12.40 €
 * @endcode
 *
 * Ja lopuksi raportti tulostetaan
 *
 * @code
 *    raportti.tulosta( &printer );
 * @endcode
 *
 */
class RaportinKirjoittaja
{

public:
    RaportinKirjoittaja();

    void asetaOtsikko(const QString& otsikko);
    void asetaKausiteksti(const QString& kausiteksti);

    /**
     * @brief Lisää sarakkeen
     * @param leveysteksti Teksti, jolle sarake mitoitetaan
     */
    void lisaaSarake(const QString& leveysteksti);
    /**
     * @brief Lisää sarakkeen
     * @param leveysprosentti Sarakkeen leveys on % sivun leveydestä
     */
    void lisaaSarake(int leveysprosentti);

    /**
     * @brief Lisää venyvän sarakkeen
     * @param tekija Missä suhteessa jäljellä oleva tila jaetaan
     */
    void lisaaVenyvaSarake(int tekija = 100);

    /**
     * @brief Lisää euromääräisen sarakkeen
     */
    void lisaaEurosarake();
    /**
     * @brief Lisää sarakkeen päivämäärälle
     */
    void lisaaPvmSarake();

    void lisaaOtsake( RaporttiRivi otsikkorivi);
    void lisaaRivi( RaporttiRivi rivi = RaporttiRivi());

    /**
     * @brief Tulostaa kirjoitetun raportin
     * @param printer
     * @param alkusivunumero Ensimmäisen tulostettavan sivun numero
     * @return sivuja tulostettu
     */
    int tulosta(QPrinter *printer, bool raidoita = false, int alkusivunumero = 1);


signals:

public slots:

protected:
    void tulostaYlatunniste(QPainter *painter, int sivu);


protected:
    QString otsikko_;
    QString kausiteksti_;

    QList<RaporttiSarake> sarakkeet_;
    QList<RaporttiRivi> otsakkeet_;
    QList<RaporttiRivi> rivit_;

};

#endif // RAPORTINKIRJOITTAJA_H
