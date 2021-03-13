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
#ifndef KPKPLEDIT_H
#define KPKPLEDIT_H

#include <QLineEdit>

/**
 * @brief Kappalemäärän muokkausrivi
 *
 * Mahdollistaa äärivaltaisen tarkan desimaaliluvun
 * kirjoittamisen ja tallentamisen merkkijonona.
 *
 * Näytettäessä käytetään desimaalipilkkua mutta
 * tallennettaessa desimaalipistettä.
 *
 * Syötettäessä sallitaan
 * - numerot
 * - yksi desimaalipiste tai -pilkku
 * - +/- merkit vaihtavat etumerkkiä
 *
 */
class KpKplEdit : public QLineEdit
{
    Q_OBJECT
public:
    KpKplEdit(QWidget* parent = nullptr);

    void setText(const QString& value);
    QString text() const;
    double kpl() const;

protected:
    void keyPressEvent(QKeyEvent* event);
};

#endif // KPKPLEDIT_H
