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
#ifndef TOSITELUETTELO_H
#define TOSITELUETTELO_H

#include "raportteri.h"
#include "db/tilikausi.h"

class TositeLuettelo : public Raportteri
{
    Q_OBJECT
public:
    TositeLuettelo(QObject *parent = nullptr);

    void kirjoita(const QDate& mista, const QDate& mihin,
                  int optiot = 0);

    enum { TositeJarjestyksessa = 0b00001 ,
           RyhmitteleLajeittain = 0b00010 ,
           TulostaKohdennukset  = 0b00100 ,
           TulostaSummat        = 0b01000 ,
           SamaTilikausi        = 0b10000,
           AsiakasToimittaja    = 0b100000
         };

private slots:
    void dataSaapuu(QVariant *data);

private:
    int optiot_ = 0;


};

#endif // TOSITELUETTELO_H
