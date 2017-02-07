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

#ifndef KOHDENNUS_H
#define KOHDENNUS_H

#include <QString>
#include <QDate>

/**
 * @brief Projektin luokka
 */

class Kohdennus
{
public:
    Kohdennus(const QString& nimi = QString());
    Kohdennus(int id, QString nimi, QDate alkaa, QDate paattyy);

    int id() const { return id_; }
    QString nimi() const { return nimi_; }
    QDate alkaa() const { return alkaa_; }
    QDate paattyy() const { return paattyy_; }

    bool muokattu() const { return muokattu_; }

    void asetaId(int id);
    void asetaNimi(const QString& nimi);
    void asetaAlkaa(const QDate& alkaa);
    void asetaPaattyy(const QDate& paattyy);
    void nollaaMuokattu();

protected:
    int id_;
    QString nimi_;
    QDate alkaa_;
    QDate paattyy_;
    bool muokattu_;
};

#endif // KOHDENNUS_H
