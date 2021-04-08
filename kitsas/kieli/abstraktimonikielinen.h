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
#ifndef ABSTRAKTIMONIKIELINEN_H
#define ABSTRAKTIMONIKIELINEN_H

#include <QVariantMap>

class AbstraktiMonikielinen
{
public:    

    virtual void aseta(const QVariant& var) = 0;
    virtual void aseta(const QString& nimi, const QString& kieli) = 0;    

    virtual QString teksti(const QString& kieli = QString()) const = 0;
    virtual QString kaannos(const QString& kieli) const = 0;

    virtual QVariantMap map() const = 0;
};

#endif // ABSTRAKTIMONIKIELINEN_H
