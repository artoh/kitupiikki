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

/**
  * @dir kirjaus
  * @brief Uuden tositteen luominen tai tositteen muokkaaminen
  */

#ifndef KIRJAUSSIVU_H
#define KIRJAUSSIVU_H

#include <QWidget>

#include "kitupiikkisivu.h"

class KirjausWg;
class QSplitter;
class KitupiikkiIkkuna;
class SelausWg;

class NaytaLiiteWidget;

/**
 * @brief Sivu, jolla kirjaukset tehdään
 *
 * Sivun yläpuolistkolla on NaytaliiteWg liiteen esittämiseen ja lisäämiseen,
 * alapuoliskolla KirjausWg kirjauksen muokkaamiseen
 *
 * naytatosite-funktiolla saadaan tosite muokattavaksi, ja muokkaamisen jälkeen
 * palataan edelliselle sivulle
 *
 */
class KirjausSivu : public KitupiikkiSivu
{
    Q_OBJECT
public:
    enum Takaisinpaluu { EI_PALATA, PALATAAN_HYLATYSTA, PALATAAN_AINA };

    KirjausSivu(KitupiikkiIkkuna *ikkuna, QList<int> selausLista = QList<int>());
    ~KirjausSivu();

    void siirrySivulle();
    bool poistuSivulta(int minne);

    QString ohjeSivunNimi() { return "kirjaus"; }

    KirjausWg* kirjausWg() { return kirjauswg; }

signals:
    /**
     * @brief Palataan sivulle, josta tänne on tultu
     */
    void palaaEdelliselleSivulle();

public slots:
    void naytaTosite(int tositeId, int tositetyyppi = -1, QList<int> selausLista = QList<int>(), Takaisinpaluu takaisinpaluu = PALATAAN_AINA);
    void tositeKasitelty(bool tallennettu);
    /**
     * @brief Kun splitteriä säädetään, talletaan asetus
     */
    void talletaSplitter();

    /**
     * @brief Lisää ensimmäisen tiedoston kirjattavien kansiosta
     */
    void lisaaKirjattavienKansiosta();

protected:
    KitupiikkiIkkuna *ikkuna_;
    KirjausWg *kirjauswg;
    NaytaLiiteWidget *liitewg;
    QSplitter *splitter;


    /**
     * @brief Palataanko tämän tositteen käsittelyn jälkeen takaisin edelliseen
     */
    Takaisinpaluu palataanTakaisin_ = EI_PALATA;
};

#endif // KIRJAUSSIVU_H
