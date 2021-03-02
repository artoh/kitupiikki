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
#ifndef MONIKIELINEN_H
#define MONIKIELINEN_H

#include "abstraktimonikielinen.h"

class Monikielinen : public AbstraktiMonikielinen
{
public:
    Monikielinen();
    Monikielinen(const QVariant& var);
    Monikielinen(const QString& str);

public:
    virtual void aseta(const QVariant &var) override;
    virtual void aseta(const QString &nimi, const QString &kieli) override;
    virtual QString teksti(const QString &kieli = QString()) const override;
    virtual QString kaannos(const QString &kieli) const override;
    virtual QVariantMap map() const override;

protected:
    QMap<QString,QString> tekstit_;

};

#endif // MONIKIELINEN_H
