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

#ifndef POISTAJA_H
#define POISTAJA_H

#include <QDialog>

#include "db/kirjanpito.h"
#include "raportti/raportinkirjoittaja.h"

namespace Ui {
class Poistaja;
}

/**
 * @brief Suunnitelman mukaisten poistojen tekeminen
 *
 * Näyttää dialogin, jossa voi esikatsella poistoja, ja käyttäjän vahvistaessa
 * kirjaa poistot tositteella, jonka liitteeksi tulee poistolaskelma
 */
class Poistaja : public QDialog
{
    Q_OBJECT

public:
    explicit Poistaja(QWidget *parent = nullptr);
    ~Poistaja();

    bool teepoistot(const Tilikausi& kausi, const QVariantList& poistot);

signals:
    void poistettu();

private:
    RaportinKirjoittaja poistoehdotus(const Tilikausi& kausi, const QVariantList& poistot);


    Ui::Poistaja *ui;
};

#endif // POISTAJA_H
