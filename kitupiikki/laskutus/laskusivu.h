/*
   Copyright (C) 2018 Arto Hyvättinen

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

#ifndef LASKUSIVU_H
#define LASKUSIVU_H

/**
  * @dir laskutus
  * @brief Laskujen laatiminen, selaaminen, maksaminen ja modelit
  */

#include "kitupiikkisivu.h"

class QTabBar;
class QSplitter;
class QLineEdit;

class KumppaniTuoteWidget;
class LaskulistaWidget;

/**
 * @brief Laskusivu
 *
 * Laskujen luettelo, näyttäminen, hyvityslaskun tekeminen, uuden laskun tekeminen jne.
 *
 *  @since 1.1
 *  Korvaa aiemman LaskutusSivun. Käyttöliittymä luodaan käsin (ilman ui-tiedostoa), jotta QSplitter
 *  saadaan toimimaan
 */
class LaskuSivu : public KitupiikkiSivu
{
public:
    LaskuSivu();
    ~LaskuSivu() override;

    enum PaaLehdet { MYYNTI, OSTO, REKISTERI, ASIAKAS, TOIMITTAJA, TUOTTEET };
    enum LajiLehdet { LUONNOKSET, LAHETETTAVAT, KAIKKI, AVOIMET, ERAANTYNEET };

    void siirrySivulle() override;
    QString ohjeSivunNimi() override { return "laskutus"; }

public slots:
    void paaTab(int indeksi);
/*    void paivitaAsiakasSuodatus();
    void paivitaLaskulista();
    void asiakasValintaMuuttuu();
    void laskuValintaMuuttuu();

    void uusiLasku();
    void naytaLasku();

    void hyvityslasku();
    void kopioiLasku();
    void muokkaaLaskua();
    void maksumuistutus();
    void poistaLasku();

private slots:
    void naytaLaskuDlg(QVariant* data);
*/

private:
    void luoUi();

    QTabBar *paaTab_;
    QSplitter *splitter_;
    QLineEdit* asiakasSuodatusEdit_;

    KumppaniTuoteWidget* kumppaniTuoteWidget_;
    LaskulistaWidget *laskuWidget_;


};

#endif // LASKUSIVU_H
