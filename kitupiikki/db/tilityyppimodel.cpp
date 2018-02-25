/*
   Copyright (C) 2017 Arto Hyvättinen

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

#include "tilityyppimodel.h"


TiliTyyppi::TiliTyyppi(int otsikkotaso)
    : luonne_( TiliLaji::OTSIKKO), otsikkotaso_(otsikkotaso)
{
    tyyppikoodi_ = QString("H%1").arg(otsikkotaso);
    kuvaus_ = QString("Otsikko %1").arg(otsikkotaso);
}

TiliTyyppi::TiliTyyppi(QString tyyppikoodi, QString kuvaus, TiliLaji::TiliLuonne luonne)
    : tyyppikoodi_(tyyppikoodi), kuvaus_(kuvaus), luonne_(luonne), otsikkotaso_(0)
{

}

bool TiliTyyppi::onko(TiliLaji::TiliLuonne luonnetta) const
{
    return ( luonne_ & luonnetta ) == luonnetta;
}



TilityyppiModel::TilityyppiModel(QObject *parent)
    : QAbstractListModel(parent)

{
    lisaa(TiliTyyppi("A","Vastaavaa",TiliLaji::VASTAAVAA));
    lisaa(TiliTyyppi("APM","Poistokelpoinen omaisuus menojäännöspoisto",TiliLaji::MENOJAANNOSPOISTO));
    lisaa(TiliTyyppi("APT","Poistokelpoinen omaisuus tasapoistolla",TiliLaji::TASAERAPOISTO));
    lisaa(TiliTyyppi("AS","Saatavaa",TiliLaji::SAATAVA));
    lisaa(TiliTyyppi("AO","Myyntisaatavat",TiliLaji::MYYNTISAATAVA));
    lisaa(TiliTyyppi("AL","Arvonlisäverosaatava", TiliLaji::ALVSAATAVA));
    lisaa(TiliTyyppi("ARK","Käteisvarat", TiliLaji::KATEINEN));
    lisaa(TiliTyyppi("ARP","Pankkitili",TiliLaji::PANKKITILI));

    lisaa(TiliTyyppi("B","Vastattavaa", TiliLaji::VASTATTAVAA));
    lisaa(TiliTyyppi("BE","Edellisten tilikausien voitto/tappio",TiliLaji::EDELLISTENTULOS));
    lisaa(TiliTyyppi("T","Tilikauden tulos", TiliLaji::KAUDENTULOS));
    lisaa(TiliTyyppi("BS","Velat",TiliLaji::VELKA));
    lisaa(TiliTyyppi("BO","Ostovelat", TiliLaji::OSTOVELKA));
    lisaa(TiliTyyppi("BL","Arvonlisäverovelka",TiliLaji::ALVVELKA));
    lisaa(TiliTyyppi("BV","Verovelka",TiliLaji::VEROVELKA));

    lisaa(TiliTyyppi("C","Tulot",TiliLaji::TULO));
    lisaa(TiliTyyppi("CL","Liikevaihtotulo (myynti)", TiliLaji::LVTULO));
    lisaa(TiliTyyppi("D","Menot",TiliLaji::MENO));
    lisaa(TiliTyyppi("DP","Poistot",TiliLaji::POISTO));


}

int TilityyppiModel::rowCount(const QModelIndex & /* parent */) const
{
    return tyypit.count();
}

QVariant TilityyppiModel::data(const QModelIndex &index, int role) const
{
    if( !index.isValid())
        return QVariant();

    TiliTyyppi tyyppi = tyypit.value( index.row());

    if( role == KuvausRooli || role == Qt::DisplayRole)
        return tyyppi.kuvaus();
    else if( role == KoodiRooli )
        return tyyppi.koodi();
    else if( role == LuonneRooli )
        return tyyppi.luonne();

    return QVariant();
}

TiliTyyppi TilityyppiModel::tyyppiKoodilla(QString koodi)
{
    if( koodi.startsWith('H'))
    {
        // Otsikko
        return TiliTyyppi( koodi.mid(1).toInt());
    }

    foreach (TiliTyyppi tyyppi, tyypit) {
        if( tyyppi.koodi() == koodi )
            return tyyppi;
    }
    return TiliTyyppi(koodi);
}

void TilityyppiModel::lisaa(TiliTyyppi tyyppi)
{
    tyypit.append(tyyppi);
}
