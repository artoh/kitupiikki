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
#ifndef TILIOTEHARMAARIVI_H
#define TILIOTEHARMAARIVI_H

#include "tilioterivi.h"
#include "model/tositevienti.h"

class TilioteHarmaaRivi : public TilioteRivi
{
public:
    TilioteHarmaaRivi();
    TilioteHarmaaRivi(const QVariantMap& data, TilioteModel* model);


    QVariant riviData(int sarake, int role=Qt::DisplayRole) const;

    TositeVienti vienti() const;

protected:
    TositeVienti vienti_;
};

#endif // TILIOTEHARMAARIVI_H
