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

#include "kielet.h"

#include <QIcon>

KieliCombo::KieliCombo(QWidget *parent) :
    QComboBox(parent)
{
    alusta();
}

void KieliCombo::alusta()
{
    clear();
    auto lista = Kielet::instanssi()->kielet();
    for(auto const &kieli : qAsConst(lista)) {
        addItem(QIcon(kieli.lippu()), kieli.nimi(), kieli.lyhenne());
    }

    setCurrentIndex( findData( Kielet::instanssi()->nykyinen() ) );
}

void KieliCombo::valitse(const QString &kielikoodi)
{
    int index = findData( kielikoodi );
    if( index > -1)
        setCurrentIndex( index );
}

QString KieliCombo::kieli() const
{
    return currentData().toString();
}
