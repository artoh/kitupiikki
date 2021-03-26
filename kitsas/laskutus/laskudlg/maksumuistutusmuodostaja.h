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
#ifndef MAKSUMUISTUTUSMUODOSTAJA_H
#define MAKSUMUISTUTUSMUODOSTAJA_H

#include "model/euro.h"
#include "model/tosite.h"

class KitsasInterface;
class Tosite;

class MaksumuistutusMuodostaja
{
public:
    MaksumuistutusMuodostaja(KitsasInterface* kitsas);

    void muodostaMuistutukset(Tosite* tosite,
                              const QDate& pvm,
                              int eraId,
                              Euro maksumuistus = Euro(0),
                              Euro korkoSaldo = Euro(0),
                              const QDate& korkoAlkaa = QDate(),
                              const QDate& korkoLoppuu = QDate(),
                              double korko = 0.0);

    Euro laskeKorko(Euro korkoSaldo = Euro(0),
                    const QDate& korkoAlkaa = QDate(),
                    const QDate& korkoLoppuu = QDate(),
                    double korko = 0.0);

protected:
    void muistutusMaksu(Tosite* tosite, Euro maksu, const QDate& pvm);
    void kirjaaKorko(Tosite* tosite, Euro korkosaldo,
               const QDate& alkupvm, const QDate& loppupvm,
               double korko, const QDate& pvm);

    double paivaKorko(const QDate& alkupvm, const QDate& loppupvm, double korko, Euro peruste);
    void vastakirjaus(Tosite *tosite, const Euro& euro, int era, const QDate &pvm);


    KitsasInterface* kitsas_;
};

#endif // MAKSUMUISTUTUSMUODOSTAJA_H
