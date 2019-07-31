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
#ifndef ASIAKAS_H
#define ASIAKAS_H

#include "kantaasiakastoimittaja.h"


class Asiakas : public KantaAsiakasToimittaja
{
    Q_OBJECT
public:
    explicit Asiakas(QObject *parent = nullptr);

    QString ovt() const { return data_.value("ovt").toString();}
    QString operaattori() const { return data_.value("operaattori").toString();}



public slots:
    void lataa(int id);
    void clear();
    void tallenna();

protected:

};

#endif // ASIAKAS_H
