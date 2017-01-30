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

#ifndef TOSITETYYPPI_H
#define TOSITETYYPPI_H

#include <QString>

class TositeTyyppi
{
public:
    TositeTyyppi(const QString& tunnus, const QString nimi);

    QString tunnus() const { return tunnus_; }
    QString nimi() const { return nimi_; }

    bool onkoKaytettavissa() const;

protected:
    QString tunnus_;
    QString nimi_;

};

#endif // TOSITETYYPPI_H
