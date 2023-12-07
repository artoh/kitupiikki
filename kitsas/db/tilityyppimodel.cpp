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

TiliTyyppi::TiliTyyppi(QString tyyppikoodi, QString kuvaus, TiliLaji::TiliLuonne luonne, bool uniikki)
    : tyyppikoodi_(tyyppikoodi), kuvaus_(kuvaus), luonne_(luonne), otsikkotaso_(0), uniikki_(uniikki)
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
    lisaa(TiliTyyppi("AJ","Siirtosaamiset",TiliLaji::SIIRTOSAATAVA));
    lisaa(TiliTyyppi("AL","Arvonlisäverosaatava", TiliLaji::ALVSAATAVA, true));
    lisaa(TiliTyyppi("ALM","Maksuperusteisen alv:n kohdentamaton saatava", TiliLaji::KOHDENTAMATONALVSAATAVA, true));
    lisaa(TiliTyyppi("AV","Verosaatava",TiliLaji::VEROSAATAVA, true));
    lisaa(TiliTyyppi("ARK","Käteisvarat", TiliLaji::KATEINEN));
    lisaa(TiliTyyppi("ARP","Pankkitili",TiliLaji::PANKKITILI));

    lisaa(TiliTyyppi("B","Vastattavaa", TiliLaji::VASTATTAVAA));
    lisaa(TiliTyyppi("BE","Edellisten tilikausien voitto/tappio",TiliLaji::EDELLISTENTULOS, true));
    lisaa(TiliTyyppi("T","Tilikauden tulos", TiliLaji::KAUDENTULOS, true));
    lisaa(TiliTyyppi("BS","Velat",TiliLaji::VELKA));
    lisaa(TiliTyyppi("BSP", "Luottotili", TiliLaji::VELKATILI));
    lisaa(TiliTyyppi("BO","Ostovelat", TiliLaji::OSTOVELKA));
    lisaa(TiliTyyppi("BJ","Siirtovelat",TiliLaji::SIIRTOVELKA));
    lisaa(TiliTyyppi("BL","Arvonlisäverovelka",TiliLaji::ALVVELKA, true));
    lisaa(TiliTyyppi("BLM","Maksuperusteisen alv:n kohdentamaton velka", TiliLaji::KOHDENTAMATONALVVELKA, true));
    lisaa(TiliTyyppi("BV","Verovelka",TiliLaji::VEROVELKA, true));
    lisaa(TiliTyyppi("BY","Yksityistilit",TiliLaji::VASTATTAVAA));

    lisaa(TiliTyyppi("C","Tulot",TiliLaji::TULO));
    lisaa(TiliTyyppi("CL","Liikevaihtotulo (myynti)", TiliLaji::LVTULO));
    lisaa(TiliTyyppi("CZ","Verottomat tulot",TiliLaji::TULO));
    lisaa(TiliTyyppi("CLZ","Veroton myynti", TiliLaji::LVTULO));
    lisaa(TiliTyyppi("D","Menot",TiliLaji::MENO));
    lisaa(TiliTyyppi("DP","Poistot",TiliLaji::POISTO));
    lisaa(TiliTyyppi("DZ","Vähennyskelvottomat menot",TiliLaji::MENO));
    lisaa(TiliTyyppi("DH","Puoliksi vähennyskelpoiset menot",TiliLaji::MENO));
    lisaa(TiliTyyppi("DPZ","Vähennyskelvottomat poistot",TiliLaji::POISTO));

    lisaa(TiliTyyppi("DVE", "Ennakkoverot", TiliLaji::MENO));


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
    else if( role == UniikkiRooli)
        return tyyppi.onkoUniikki();

    return QVariant();
}

TiliTyyppi TilityyppiModel::tyyppiKoodilla(const QString& koodi)
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

void TilityyppiModel::lisaa(const TiliTyyppi& tyyppi)
{
    tyypit.append(tyyppi);
}
