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
#include "tositevienti.h"

TositeVienti::TositeVienti(const QVariantMap &vienti) :
    QVariantMap (vienti)
{

}

QVariant TositeVienti::data(int kentta) const
{
    return value( avaimet__.at(kentta) );
}

void TositeVienti::set(int kentta, const QVariant &arvo)
{
    if( arvo.toString().isEmpty())
        remove( avaimet__.at(kentta));
    else
        insert( avaimet__.at(kentta), arvo);
}

QList<int> TositeVienti::merkkaukset() const
{
    QList<int> lista;
    for(auto merkkaus : data(MERKKAUKSET).toList())
        lista.append( merkkaus.toInt());
    return lista;
}

void TositeVienti::setPvm(const QDate &pvm)
{
    set( PVM, pvm );
}

void TositeVienti::setTili(int tili)
{
    set( TILI, tili);
}

void TositeVienti::setDebet(double euroa)
{
    set( DEBET, euroa);
    if( qAbs(euroa) < 1e-5)
        set(KREDIT, 0);
}

void TositeVienti::setDebet(qlonglong senttia)
{
    setDebet( senttia / 100.0);
}

void TositeVienti::setKredit(double euroa)
{
    set( KREDIT, euroa );
    if( qAbs(euroa) > 1e-5)
        set( DEBET, 0);
}

void TositeVienti::setKredit(qlonglong senttia)
{
    setKredit( senttia / 100.0);
}

void TositeVienti::setSelite(const QString &selite)
{
    set( SELITE, selite);
}

void TositeVienti::setAlvKoodi(int koodi)
{
    set( ALVKOODI, koodi);
}

void TositeVienti::setAlvProsentti(double prosentti)
{
    set( ALVPROSENTTI, prosentti);
}

void TositeVienti::setKohdennus(int kohdennus)
{
    set( KOHDENNUS, kohdennus);
}

void TositeVienti::setMerkkaukset(QVariantList merkkaukset)
{
    if( merkkaukset.isEmpty())
        remove( avaimet__.at(MERKKAUKSET));
    else
        insert( avaimet__.at(MERKKAUKSET), merkkaukset);
}

void TositeVienti::setJaksoalkaa(const QDate &pvm)
{
    set( JAKSOALKAA, pvm);
}

void TositeVienti::setJaksoloppuu(const QDate &pvm)
{
    set( JAKSOLOPPUU, pvm );
}

void TositeVienti::setEra(int era)
{
    set( ERAID, era);
}

void TositeVienti::setArkistotunnus(const QString &tunnus)
{
    set( ARKISTOTUNNUS, tunnus);
}






std::map<int,QString> TositeVienti::avaimet__ = {
    { ID, "id" },
    { PVM, "pvm"},
    { TILI, "tili"},
    { DEBET, "debet"},
    { KREDIT, "kredit"},
    { SELITE, "selite"},
    { ALVKOODI, "alvkoodi"},
    { ALVPROSENTTI, "alvprosentti"},
    { KOHDENNUS, "kohdennus"},
    { MERKKAUKSET, "merkkaukset"},
    { JAKSOALKAA, "jaksoalkaa"},
    { JAKSOLOPPUU, "jaksoloppuu"},
    { ERAID, "era"},
    { ARKISTOTUNNUS, "arkistotunnus"}
};
