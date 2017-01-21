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

#ifndef TILISAILO_H
#define TILISAILO_H

#include <QObject>

#include "tili.h"
#include <QMap>
#include <QList>

/**
 * @brief Tilien säiliö
 */
class TiliSailo : public QObject
{
    Q_OBJECT
public:
    enum Tyyppisuodatin { KAIKKI, TASE, TULOS, TULO, MENO } ;


    explicit TiliSailo(QObject *parent = 0);
    ~TiliSailo();

    Tili* tili(int tilinumero) { return hakutaulu[tilinumero]; }

    QList<Tili*> tilit(Tyyppisuodatin tyyppisuodatin = KAIKKI, int tilasuodatin = 0,
                       int otsikkosuodatin = -1);


signals:

public slots:
    void tyhjenna();
    /**
     * @brief Indeksoi haettaessa käytettävät taulut
     *
     * Tätä funktiota pitää kutsua joka kerta sen jälkeen, kun jonkun tilin
     * numeroa on muutettu tai tilejä on luotu/ladattu tai poistettu
     */
    void indeksoi();

protected:
    QList<Tili*> tililista;
    QMap<int,Tili*> kasijarjestyslista;
    QHash<int,Tili*> hakutaulu;
};

#endif // TILISAILO_H
