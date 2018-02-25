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


/**
  * @dir tuonti
  * @brief Tiedostojen tuonti kirjanpitoon
  *
  */



#ifndef TUONTI_H
#define TUONTI_H

#include <QString>

#include "db/tositelajimodel.h"

class KirjausWg;

/**
 * @brief Tiedostojen tuonti
 */
class Tuonti
{
protected:
    Tuonti(KirjausWg *wg);

public:
    virtual bool tuoTiedosto(const QString& tiedostonnimi) = 0;

    /**
     * @brief Yrittää tuoda halutun tiedoston
     * @param tiedostonnimi Polku tiedostoon
     * @param wg KirjausWg, johon tuodaan
     * @return tosi, jos tiedosto lisätään
     */
    static bool tuo(const QString& tiedostonnimi, KirjausWg *wg);

protected:
    KirjausWg* kirjausWg() { return kirjausWg_; }

    void lisaaLasku(qlonglong sentit,
                    QDate laskupvm, QDate erapvm, QString viite,
                    QString tilinumero, QString saajanNimi);

    KirjausWg* kirjausWg_;
};

#endif // TUONTI_H
