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
class QLabel;

class AsiakkaatModel;
class LaskutModel;
class YhteystietoWidget;

class LaskuTauluModel;
class TuoteModel;

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

    enum PaaLehdet { MYYNTI, OSTO, ASIAKAS, TOIMITTAJA, TUOTTEET };
    enum LajiLehdet { LUONNOKSET, LAHETETTAVAT, KAIKKI, AVOIMET, ERAANTYNEET };

    void siirrySivulle() override;
    QString ohjeSivunNimi() override { return "laskutus"; }

public slots:
    void paaTab(int indeksi);
    void paivitaAsiakasSuodatus();
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

private:
    void luoUi();

    LaskuTauluModel* laskumodel_ = nullptr;
    AsiakkaatModel* asiakasmodel_;
    TuoteModel* tuotemodel_;


    QSortFilterProxyModel* asiakasProxy_ = nullptr;
    QSortFilterProxyModel* laskuAsiakasProxy_;
    QSortFilterProxyModel* laskuViiteProxy_;
    QSortFilterProxyModel* tuoteProxy_;

    QTabBar *paaTab_;
    QTabBar *lajiTab_;

    QSplitter *splitter_;

    QTableView* asiakasView_;
    QTableView* laskuView_;
    QTableView* tuoteView_;

    QLineEdit* asiakasSuodatusEdit_;
    QLineEdit* viiteSuodatusEdit_;

    QDateEdit* mistaEdit_;
    QLabel* viivaLabel_;
    QDateEdit* mihinEdit_;

    QPushButton* naytaNappi_;
    QPushButton* muokkaaNappi_;
    QPushButton* poistaNappi_;
    QPushButton* kopioiNappi_;
    QPushButton* hyvitysNappi_;
    QPushButton* muistutusNappi_;



};

#endif // LASKUSIVU_H
