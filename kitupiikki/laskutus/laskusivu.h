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
class QTableView;
class QPushButton;
class QLineEdit;
class QSortFilterProxyModel;
class QDateEdit;

class AsiakkaatModel;
class LaskutModel;

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
    ~LaskuSivu();

    enum PaaLehdet { MYYNTI, OSTO, ASIAKAS, TOIMITTAJA };
    enum LajiLehdet { KAIKKI, AVOIMET, ERAANTYNEET, TIEDOT };

    void siirrySivulle() override;
    QString ohjeSivunNimi() override { return "laskutus"; }

public slots:
    void paaTab(int indeksi);
    void paivitaAsiakasSuodatus();
    void paivitaLaskulista();
    void asiakasValintaMuuttuu();

    void uusiLasku();

private:
    void luoUi();

    AsiakkaatModel* asiakasmodel_;
    LaskutModel* laskumodel_ = 0;

    QSortFilterProxyModel* asiakasProxy_ = 0;
    QSortFilterProxyModel* laskuAsiakasProxy_;
    QSortFilterProxyModel* laskuViiteProxy_;

    QTabBar *paaTab_;
    QTabBar *lajiTab_;

    QSplitter *splitter_;

    QTableView* asiakasView_;
    QTableView* laskuView_;

    QLineEdit* asiakasSuodatusEdit_;
    QLineEdit* viiteSuodatusEdit_;

    QDateEdit* mistaEdit_;
    QDateEdit* mihinEdit_;

    QPushButton* naytaNappi_;
};

#endif // LASKUSIVU_H
