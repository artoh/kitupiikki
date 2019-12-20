/*
   Copyright (C) 2019 Arto Hyvättinen

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
#ifndef LASKULISTAWIDGET_H
#define LASKULISTAWIDGET_H

#include <QWidget>

class QSortFilterProxyModel;
class LaskuTauluModel;

namespace Ui {
class LaskulistaWidget;
}

class LaskulistaWidget : public QWidget
{
    Q_OBJECT

    enum PaaLehdet { MYYNTI, OSTO, REKISTERI, ASIAKAS, TOIMITTAJA, TUOTTEET };
    enum LajiLehdet { LUONNOKSET, LAHETETTAVAT, KAIKKI, AVOIMET, ERAANTYNEET };

public:
    explicit LaskulistaWidget(QWidget *parent = nullptr);
    ~LaskulistaWidget();

public slots:
    void nayta(int paalehti);
    void paivita();
    void suodataAsiakas(const QString& nimi);

    void paivitaNapit();

    void laheta();
    void alusta();

    void uusilasku();

private slots:
    void muokkaa();
    void kopioi();
    void hyvita();
    void poista();

    void naytaLasku();
    void naytaDialogi(QVariant* data);
    void haettuKopioitavaksi(QVariant* data);
    void teeHyvitysLasku(QVariant *data);


private:
    Ui::LaskulistaWidget *ui;

    LaskuTauluModel *laskut_;
    QSortFilterProxyModel *laskuAsiakasProxy_;
    QSortFilterProxyModel *laskuViiteProxy_;

    int paalehti_ = 0;
};

#endif // LASKULISTAWIDGET_H
