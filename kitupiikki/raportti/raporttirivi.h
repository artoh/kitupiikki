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

#ifndef RAPORTTIRIVI_H
#define RAPORTTIRIVI_H

#include <QString>
#include <QDate>
#include <QList>

/**
 * @brief Yhden sarakkeen tiedot
 */
struct RaporttiRiviSarake
{
    bool tasaaOikealle = false;

    QString teksti;
    int leveysSaraketta = 1;
};

/**
 * @brief Yksi raportin tai otsakkeen rivi
 */
class RaporttiRivi
{
public:
    RaporttiRivi();

    void lisaa(const QString& teksti, int sarakkeet = 1, bool tasaaOikealle = false);
    void lisaa(int sentit, int sarakkeet = 1, bool tulostanollat = false);
    void lisaa(const QDate& pvm, int sarakkeet = 1);

    int sarakkeita() const { return sarakkeet_.count(); }
    QString teksti(int rivi) { return sarakkeet_[rivi].teksti; }
    int leveysSaraketta(int rivi) { return sarakkeet_[rivi].leveysSaraketta; }
    bool tasattuOikealle(int rivi) { return sarakkeet_[rivi].tasaaOikealle; }



protected:
    QList<RaporttiRiviSarake> sarakkeet_;
};

#endif // RAPORTTIRIVI_H
