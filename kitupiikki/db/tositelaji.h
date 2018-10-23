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

#ifndef TOSITELAJI_H
#define TOSITELAJI_H

#include <QString>

#include "jsonkentta.h"

/**
 * @brief Tositelaji, joka muodostaa oman numerosarjan
 */
class Tositelaji
{
public:
    Tositelaji();
    Tositelaji(int id, const QString &tunnus, const QString &nimi,
               const QByteArray &json = QByteArray());

    int id() const { return id_; }
    QString tunnus() const { return tunnus_; }
    QString nimi() const { return nimi_; }
    bool muokattu() const { return muokattu_ | json_.onkoMuokattu(); }

    JsonKentta *json() { return &json_; }

    void asetaId(int id);
    void asetaTunnus(const QString& tunnus);
    void asetaNimi(const QString& nimi);
    void nollaaMuokattu();

    int montakoTositetta() const;

    /**
     * @brief Palauttaa tämän tositelajin seuraavan tunnistenumeron
     * @param pvm Päivämäärä, jonka perusteella tilikausi määräytyy
     * @return
     */
    int seuraavanTunnistenumero(const QDate pvm) const;

protected:
    int id_;
    QString tunnus_;
    QString nimi_;
    JsonKentta json_;
    bool muokattu_;
};

#endif // TOSITELAJI_H
