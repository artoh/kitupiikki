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
#ifndef TAYDENNYSVIENNIT_H
#define TAYDENNYSVIENNIT_H

#include "model/tositevienti.h"
#include <QList>

class KitsasInterface;

class TaydennysViennit
{
public:
    TaydennysViennit(KitsasInterface *interface);

    void asetaEra(int eraId, QVariantList alkuperaisviennit);
    int eraId() const { return eraId_;}

    QList<TositeVienti> viennit(const QVariantList &omatViennit) const;

    static void asetaKitsas(KitsasInterface* interface);

    void asetaDebetId(int id) { taydennysDebetId_ = id;}
    void asetaKreditId(int id) { taydennysKreditId_ = id;}

private:
    QList<TositeVienti> alkuperaiset_;
    int eraId_ = 0;

    int taydennysDebetId_ = 0;
    int taydennysKreditId_ = 0;

    KitsasInterface* kitsasInterface_;
};

#endif // TAYDENNYSVIENNIT_H
