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

#ifndef TPALOITUS_H
#define TPALOITUS_H

#include <QDialog>

#include <QStandardItemModel>
#include "db/tilikausi.h"

namespace Ui {
class TpAloitus;
}

/**
 * @brief Tilinpäätöksen valintojen dialogi
 *
 * Näyttää dialogin, jossa valitaan yrityksen koko (PMA-säännöstö ja henkilöstö) ja tilinpäätöksen valinnat
 * Vaihtoehtoisesti tilinpäätöksen voi ladata tiedostosta
 *
 * Tilinpäätöskaavassa '##Otsikko' on näytettävä väliotsikko ja ehdot muotoa
 * '#tunnus -sulkeva Näytettävä teksti'
 *
 * Ehto koskee vain mikroyritystä sulkumääreillä '-P' '-I' ja pienyritystä
 * sulkumääreillä '-M' '-I'
 *
 */
class TpAloitus : public QDialog
{
    Q_OBJECT

public:
    explicit TpAloitus(Tilikausi kausi, QWidget *parent = nullptr);
    ~TpAloitus();

    bool kaytetaankoTiedostoa() const { return kaytaTiedostoa_; }

private slots:
    void valintaMuuttui(QStandardItem *item);
    void accept();
    void lataaTiedosto();
    void tiedostoLadattu();
    void ohje();
    void lataa();

    void tarkistaPMA();

private:
    enum {
        TunnusRooli = Qt::UserRole + 1,
        PoisRooli = Qt::UserRole + 2
    };

    void talleta();

    Ui::TpAloitus *ui;
    Tilikausi tilikausi;
    QStandardItemModel *model;

    bool kaytaTiedostoa_ = false;

};

#endif // TPALOITUS_H
