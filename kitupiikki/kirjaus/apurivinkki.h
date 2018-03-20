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

#ifndef APURIVINKKI_H
#define APURIVINKKI_H

#include <QWidget>

namespace Ui {
class ApuriVinkki;
}

/**
 * @brief Vinkkiruutu Apurin käyttöön
 *
 * Ensimmäisillä kirjauskerroilla näytetään vinkkiruutu,
 * joka kehottaa käyttämään Apuria!
 */
class ApuriVinkki : public QWidget
{
    Q_OBJECT

public:
    explicit ApuriVinkki(QWidget *parent = 0);
    ~ApuriVinkki();

private:
    Ui::ApuriVinkki *ui;
};

#endif // APURIVINKKI_H
