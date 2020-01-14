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

#ifndef TARARKISTO_H
#define TARARKISTO_H

#include <QFile>
#include <QByteArray>

/**
 * @brief Tar-arkiston muodostaminen
 */
class TarArkisto : protected QFile
{
public:
    TarArkisto(const QString& polku);
    ~TarArkisto();

    /**
     * @brief Aloittaa tar-arkiston kirjoittamisen
     * @return tosi, jos onnistui
     */
    bool aloita();

    /**
     * @brief Lisää tiedoston arkistoon
     * @param polku Polku tiedostoon
     * @return tosi, jos onnistui
     */
    bool lisaaTiedosto(const QString& polku);

    /**
     * @brief Kirjoittaa päättävät kentät ja sulkee tiedoston
     * @return tosi, jos onnistui
     */
    void lopeta();

protected:

};

#endif // TARARKISTO_H
