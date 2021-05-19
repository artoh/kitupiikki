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
#ifndef HYVITYSLASKUDIALOGI_H
#define HYVITYSLASKUDIALOGI_H

#include "rivillinenlaskudialogi.h"

class HyvitysLaskuDialogi : public RivillinenLaskuDialogi
{
    Q_OBJECT
public:
    HyvitysLaskuDialogi(Tosite* tosite, QWidget* parent = nullptr);

    void asetaEra(int eraId);

protected:
    QString ohje() override { return "laskutus/myynnit/hyvityslasku/";}
    void valmisteleTallennus() override;

    int eraId_ = 0;
};

#endif // HYVITYSLASKUDIALOGI_H
