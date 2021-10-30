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
    RaporttiRivi::RivinKaytto sarakkeenKaytto = RaporttiRivi::KAIKKI;
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
 *    kirjoittaja.lisaaPvmSarake();
 *    kirjoittaja.lisaaVenyvaSarake();
 *    kirjoittaja.lisaaEuroSarake();
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
 *    kirjoittaja.lisaaRivi(rivi);
 * @endcode
 *
 * Ja lopuksi raportti tulostetaan
 *
 * @code
 *    kirjoittaja.tulosta( &printer, &painter );
 * @endcode
 *
 */
class RaportinKirjoittaja
{

public:
    RaportinKirjoittaja(bool csvKaytossa = true);

    void asetaOtsikko(const QString& otsikko);
    void asetaKausiteksti(const QString& kausiteksti);
    void asetaCsvKaytossa(const bool onko);

    /**
     * @brief Lisää sarakkeen
     * @param leveysteksti Teksti, jolle sarake mitoitetaan
     */
    void lisaaSarake(const QString& leveysteksti, RaporttiRivi::RivinKaytto kaytto = RaporttiRivi::KAIKKI);
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

    void lisaaOtsake(const RaporttiRivi &otsikkorivi);
    void lisaaRivi(const RaporttiRivi &rivi = RaporttiRivi(RaporttiRivi::EICSV));

    /**
     * @brief Lisää tyhjän rivin jo edellinen ei ollut jo tyhjä
     */
    void lisaaTyhjaRivi();

    /**
     * @brief Tulostaa kirjoitetun raportin
     * @param printer
     * @param painter
     * @param alkusivunumero Ensimmäisen tulostettavan sivun numero. Jos 0 ei tulosteta sivunumeroita.
     * @return sivuja tulostettu
     */
    int tulosta(QPagedPaintDevice *printer, QPainter *painter, bool raidoita = false, int alkusivunumero = 1) const;

    /**
     * @brief Palauttaa raportin html-muodossa
     * @return
     */
    QString html(bool linkit=false) const;

    /**
     * @brief pdf Raportti pdf-muodossa
     * @param taustaraidat Tulosta taustaraidat
     * @param kaytaA4 Tulostaa asetuksista riippumatta A4
     * @return
     */
    QByteArray pdf(bool taustaraidat = false, bool kaytaA4 = false,
                   QPageLayout* leiska = nullptr) const;

    /**
     * @brief Palauttaa raportin csv-muodossa
     *
     * Otsikosta huomioidaan vain ensimmäinen rivi. Tätä pitäisi käyttää vain sellaisiin raportteihin,
     * joissa joka rivillä on sama määrä sarakkeita.
     *
     * @return
     */
    QByteArray csv() const;

    QString otsikko() const { return otsikko_; }
    QString kausiteksti() const { return kausiteksti_; }

    bool csvKaytossa() const { return csvKaytossa_;}

    void tulostaYlatunniste(QPainter *painter, int sivu) const;

    bool tyhja() const { return rivit_.isEmpty(); }
    int riveja() const { return  rivit_.count(); }

    void asetaKieli(const QString& kieli);

signals:

public slots:

protected:
    QString kaanna(const QString& teksti) const;


protected:
    QString otsikko_;
    QString kausiteksti_;
    bool csvKaytossa_;
    QString kieli_;

    QList<RaporttiSarake> sarakkeet_;
    QList<RaporttiRivi> otsakkeet_;
    QList<RaporttiRivi> rivit_;

};

#endif // RAPORTINKIRJOITTAJA_H
