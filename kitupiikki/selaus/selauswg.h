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
  * @dir selaus
  * @brief Tositteiden ja vientien selaaminen
  */

#ifndef SELAUSWG_H
#define SELAUSWG_H

#include <QWidget>

#include "ui_selauswg.h"
#include "db/tilikausi.h"

#include "kitupiikkisivu.h"

class SelausModel;
class TositeSelausModel;
class QSortFilterProxyModel;

/**
 * @brief Sivu kirjausten selaamiseen
 *
 * Sivulla on taulukko vienneistä sekä widgetit selauksen rajaamiseen
 * ajan ja tilin perusteella
 *
 */
class SelausWg : public KitupiikkiSivu
{
    Q_OBJECT

public:
    SelausWg();
    ~SelausWg();

public slots:
    void alusta();
    void paivita();
    void suodata();
    void paivitaSummat();
    void naytaTositeRivilta(QModelIndex index);

    void selaa(int tilinumero, Tilikausi tilikausi);

    void selaaVienteja();
    void selaaTositteita();

    void alkuPvmMuuttui();

    /**
     * @brief Selaa tositteita tai vientejä
     * @param kumpi 0-tositteet, 1 viennit
     */
    void selaa(int kumpi);


public:
    void siirrySivulle() override;

    QString ohjeSivunNimi() override { return "selaus"; }

protected:
    bool eventFilter(QObject *watched, QEvent *event) override;

signals:
    void tositeValittu(int id);

private:
    Ui::SelausWg *ui;
    SelausModel *model;
    TositeSelausModel *tositeModel;

    QSortFilterProxyModel *proxyModel;
    QSortFilterProxyModel *etsiProxy;

    /**
     * @brief Pitääkö sivu päivittää ennen sen näyttämistä
     */
    bool paivitettava = true;

};

#endif // SELAUSWG_H
