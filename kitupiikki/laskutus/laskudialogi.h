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

#ifndef UUSILASKUDIALOGI_H
#define UUSILASKUDIALOGI_H

#include <QDialog>
#include <QSortFilterProxyModel>

#include "laskumodel.h"
#include "tuotemodel.h"
#include "laskuntulostaja.h"

namespace Ui {
class LaskuDialogi;
}

class LaskuDialogi : public QDialog
{
    Q_OBJECT
public:

    explicit LaskuDialogi(QWidget *parent = 0);
    ~LaskuDialogi();

private slots:
    void viewAktivoitu(QModelIndex indeksi);
    void paivitaSumma(int paivitaSumma);
    void esikatsele();
    void perusteVaihtuu();
    void haeOsoite();

    /**
     * @brief Tallentaa lomaketiedot malliin
     */
    void vieMalliin();
    void tallenna();

    void rivienKontekstiValikko(QPoint pos);
    void lisaaTuoteluetteloon();

    void lisaaTuote(const QModelIndex& index);
    void poistaLaskuRivi();

    void tuotteidenKonteksiValikko(QPoint pos);
    void poistaTuote();

private:
    /**
     * @brief Näyttäää tuoteluettelon jos tuotteita, muuten ohjeen
     */
    void paivitaTuoteluettelonNaytto();

private:
    Ui::LaskuDialogi *ui;
    LaskuModel *model;
    TuoteModel *tuotteet;
    
    LaskunTulostaja *tulostaja;
    
    QSortFilterProxyModel *tuoteProxy;

    QModelIndex kontekstiIndeksi;
    
};

#endif // UUSILASKUDIALOGI_H
