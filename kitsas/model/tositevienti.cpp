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

#include "db/kirjanpito.h"

#include <QDebug>

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
    if( (arvo.toString().isEmpty() || arvo.toString()=="0") && !(arvo.type() == QVariant::Map && !arvo.toMap().isEmpty()))
        remove( avaimet__.at(kentta));
    else
        insert( avaimet__.at(kentta), arvo);
}

QVariant TositeVienti::tallennettava() const
{
    // Tallennettavassa tietoalkioiden (toimittaja, asiakas) tilalla on kyseiset id:t

    QVariantMap ulos(*this);

    if( ulos.contains("asiakas") && ulos.value("asiakas").toMap().contains("id"))
        ulos.insert("asiakas", ulos.value("asiakas").toMap().value("id"));

    if( ulos.contains("toimittaja") && ulos.value("toimittaja").toMap().contains("id"))
        ulos.insert("toimittaja", ulos.value("toimittaja").toMap().value("id"));


    return ulos;
}

QVariantMap TositeVienti::era() const
{
    return value("era").toMap();
}

int TositeVienti::eraId() const
{
    return era().value("id").toInt();
}

int TositeVienti::kumppaniId() const
{
    return value("kumppani").toMap().value("id").toInt();
}

QVariantList TositeVienti::merkkaukset() const
{
    return data(MERKKAUKSET).toList();
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
    if( qAbs(euroa) > 1e-5)
        remove( avaimet__.at(KREDIT) );
}

void TositeVienti::setDebet(qlonglong senttia)
{
    setDebet( senttia / 100.0);
}

void TositeVienti::setKredit(double euroa)
{
    set( KREDIT, euroa );
    if( qAbs(euroa) > 1e-5)
        remove( avaimet__.at(DEBET) );
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
    if( kp()->alvTyypit()->nollaTyyppi(koodi) ) {
        remove( avaimet__.at(ALVPROSENTTI) );
    }
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
    QVariantMap map;
    map.insert("id",era);
    setEra(map);
}

void TositeVienti::setEra(const QVariantMap &era)
{
    if( era.isEmpty())
        remove(avaimet__.at(ERA));
    else
        insert(avaimet__.at(ERA), era);
}

void TositeVienti::setArkistotunnus(const QString &tunnus)
{
    set( ARKISTOTUNNUS, tunnus);
}

void TositeVienti::setKumppani(int kumppaniId)
{
    if(!kumppaniId) {
        remove(avaimet__.at(KUMPPANI));
    } else {
        QVariantMap kmap;
        kmap.insert("id", kumppaniId);
        set( KUMPPANI, kmap );
    }
}

void TositeVienti::setKumppani(const QString &kumppani)
{
    QVariantMap kmap;
    kmap.insert("nimi",kumppani);
    set( KUMPPANI, kmap);
}

void TositeVienti::setKumppani(const QVariantMap &map)
{
    set( KUMPPANI, map);
}

void TositeVienti::setTyyppi(int tyyppi)
{
    set( TYYPPI, tyyppi);
}

void TositeVienti::setPalkkakoodi(const QString &palkkakoodi)
{
    set( PALKKAKOODI, palkkakoodi);
}

void TositeVienti::setTasaerapoisto(int kuukautta)
{
    set( TASAERAPOISTO, kuukautta);
}

void TositeVienti::setId(int id)
{
    set( ID, id);
}

void TositeVienti::setBrutto(double brutto)
{
    set( BRUTTO, brutto);
}

void TositeVienti::setBrutto(qlonglong brutto)
{
    setBrutto( brutto / 100.0);
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
    { ERA, "era"},
    { ARKISTOTUNNUS, "arkistotunnus"},
    { KUMPPANI, "kumppani"},
    { TYYPPI, "tyyppi"},
    { PALKKAKOODI, "palkkakoodi"},
    { TASAERAPOISTO, "tasaerapoisto"},
    { BRUTTO, "brutto"},
    { ALKUPVIENNIT, "alkupviennit"},
    { VIITE, "viite"}
};
