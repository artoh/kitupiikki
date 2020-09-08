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

#ifndef VERODIALOGI_H
#define VERODIALOGI_H

#include <QDialog>

namespace Ui {
class VeroDialogi;
}

/**
 * @brief Verokirjauksen valintaan
 *
 * Tällä voidaan valita kaikki mahdolliset verokirjaukset
 * - Alv-koodi
 * - Alv-prosentti
 * - Kirjauksen tyyppi (peruste, vero, vähennys)
 */
class VeroDialogi : public QDialog
{
    Q_OBJECT

public:
    explicit VeroDialogi(QWidget *parent = 0);
    ~VeroDialogi();

    int alvProsentti() const;
    int alvKoodi() const;

    /**
     * @brief Näyttää dialogin annetuilla alkutiedoilla
     * @param koodi Alv-koodi
     * @param prosentti Alv-prosentti
     * @param tyyppilukko Jos kirjaustyyppiä ei saa muuttaa
     * @return QDialog::exec() paluuarvo
     */
    int nayta(int koodi, int prosentti, bool tyyppilukko = false);

public slots:
    void lajimuuttuu();

private:
    Ui::VeroDialogi *ui;

};

#endif // VERODIALOGI_H
