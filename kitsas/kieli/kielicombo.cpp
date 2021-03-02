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
#include "kielicombo.h"

#include "abstraktikielet.h"

#include <QIcon>

KieliCombo::KieliCombo()
{

}

void KieliCombo::alusta(const AbstraktiKielet *kielet)
{
    clear();
    auto lista = kielet->kielet();
    for(auto const &kieli : lista) {
        addItem(QIcon(kieli.lippu()), kieli.nimi(), kieli.lyhenne());
    }

    setCurrentIndex( findData( kielet->nykyinen() ) );
}

QString KieliCombo::kieli() const
{
    return currentData().toString();
}
