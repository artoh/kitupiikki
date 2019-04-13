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
#ifndef PILVIKYSELY_H
#define PILVIKYSELY_H

#include "db/kpkysely.h"

class PilviYhteys;

class PilviKysely : public KpKysely
{
    Q_OBJECT
public:
    PilviKysely(PilviYhteys* parent, Metodi metodi=GET, QString polku = QString());

public slots:
    void kysy(const QVariant& data = QVariant()) override;

};

#endif // PILVIKYSELY_H
