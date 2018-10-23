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
    explicit Poistaja(QWidget *parent = 0);
    ~Poistaja();

    /**
     * @brief Tee kaudelle kuuluvat sumu-poistot
     * @param kausi
     * @return Tehtiinkö poistot
     */
    static bool teeSumuPoistot(Tilikausi kausi);

    /**
     * @brief Voiko tälle kaudelle tehdä sumu-poistoja
     *
     * Kaudelle voi tehdä poistoja, jos niitä ei ole vielä tehty ja
     * jollain poistotilillä on vielä saldoa jäljellä
     *
     * @param kausi
     * @return Tosi, jos poistoja voi tehdä
     */
    bool static onkoPoistoja(const Tilikausi &kausi);

private:
    bool sumupoistaja(Tilikausi kausi);


    Ui::Poistaja *ui;
};

#endif // POISTAJA_H
