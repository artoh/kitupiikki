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

#ifndef KIRJAUSSIVU_H
#define KIRJAUSSIVU_H

#include <QWidget>

#include "kitupiikkisivu.h"
#include "db/tositemodel.h"

class KirjausWg;
class NaytaliiteWg;

class KirjausSivu : public KitupiikkiSivu
{
    Q_OBJECT
public:
    KirjausSivu();
    ~KirjausSivu();

    void siirrySivulle();
    bool poistuSivulta();

signals:

public slots:
    void naytaTosite(int tositeId);
    void tositeKasitelty();

protected:
    KirjausWg *kirjauswg;
    NaytaliiteWg *liitewg;

    TositeModel *model;

    /**
     * @brief Palataanko tämän tositteen käsittelyn jälkeen takaisin edelliseen
     */
    bool palataanTakaisin_;
};

#endif // KIRJAUSSIVU_H
