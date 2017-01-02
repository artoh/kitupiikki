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

#ifndef UUSIKIRJANPITO_H
#define UUSIKIRJANPITO_H

#include <QWizard>


/**
 * @brief Uuden kirjanpidon luominen
 *
 * Uusi kirjanpito luodaan aloitaUusiKirjanpito()-funktiolla, joka näyttää
 * valintavelhon ja luo tarvittavat tiedostot.
 *
 */
class UusiKirjanpito : public QWizard
{
protected:
    UusiKirjanpito();

public:
    /**
     * @brief Näyttää valintavelhon ja luo uuden kirjanpidon
     * @return Polku uuden kirjanpidon hakemistoon tai null, ellei luominen onnistunut
     */
    static QString aloitaUusiKirjanpito();

};

#endif // UUSIKIRJANPITO_H
