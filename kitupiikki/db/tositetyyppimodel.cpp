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
#include "tositetyyppimodel.h"

TositeTyyppiTietue::TositeTyyppiTietue(TositeTyyppi::Tyyppi uKoodi, const QString &uNimi, const QString &uKuvake, bool uLisattavissa) :
    koodi(uKoodi), nimi( uNimi ), kuvake( QIcon( uKuvake )), lisattavissa( uLisattavissa)
{

}



TositeTyyppiModel::TositeTyyppiModel(QObject *parent)
    : QAbstractListModel (parent)
{
    lisaa(TositeTyyppi::MENO, tr("Menotosite"), "poista");
    lisaa(TositeTyyppi::TULO, tr("Tulotosite"), "lisaa");

    lisaa(TositeTyyppi::KULULASKU, tr("Kululasku"), "tekstisivu", true);

    lisaa(TositeTyyppi::MYYNTILASKU, tr("Myyntilasku"), "lasku", false);
    lisaa(TositeTyyppi::HYVITYSLASKU, tr("Hyvityslasku"), "lasku", false);
    lisaa(TositeTyyppi::MAKSUMUISTUTUS, tr("Maksumuistutus"), "lasku", false);


    lisaa(TositeTyyppi::SIIRTO, tr("Rahoitustapahtuma"), "siirra");
    lisaa(TositeTyyppi::TILIOTE, tr("Tiliote"), "tekstisivu");
    lisaa(TositeTyyppi::PALKKA, tr("Palkanmaksu"), "yrittaja");
    lisaa(TositeTyyppi::MUISTIO, tr("Muistiotosite"),"kommentti");
    lisaa(TositeTyyppi::LIITETIETO, tr("Liitetietotosite"), "liite");

    lisaa(TositeTyyppi::MUU, tr("Muu"), "tyhja");

    lisaa(TositeTyyppi::TILINAVAUS, tr("Tilinavaus"), "rahaa", false);
    lisaa(TositeTyyppi::ALVLASKELMA, tr("Alv-laskelma"), "verotilitys", false);
    lisaa(TositeTyyppi::POISTOLASKELMA, tr("Poistolaskelma"), "kirjalaatikko", false);
    lisaa(TositeTyyppi::JAKSOTUS, tr("Jaksotus"), "ratas", false);
}

int TositeTyyppiModel::rowCount(const QModelIndex & /* parent */) const
{
    return tyypit_.count();
}

QVariant TositeTyyppiModel::data(const QModelIndex &index, int role) const
{
    TositeTyyppiTietue tietue = tyypit_.value(index.row());

    if( role == Qt::DisplayRole || role == NimiRooli)
        return tietue.nimi;
    else if( role == KoodiRooli )
        return tietue.koodi;
    else if( role == Qt::DecorationRole)
        return tietue.kuvake;
    else if( role == LisattavissaRooli)
        return tietue.lisattavissa ? "K" : "E" ;

    return QVariant();
}

QString TositeTyyppiModel::nimi(int koodi) const
{
    return map_.value(koodi).nimi;
}

QIcon TositeTyyppiModel::kuvake(int koodi) const
{
    return map_.value(koodi).kuvake;
}

bool TositeTyyppiModel::onkolisattavissa(int koodi) const
{
    return map_.value(koodi).lisattavissa;
}



void TositeTyyppiModel::lisaa(TositeTyyppi::Tyyppi koodi, const QString &nimi, const QString &kuvake, bool lisattavissa)
{
    TositeTyyppiTietue tietue(koodi, nimi, ":/pic/" + kuvake + ".png", lisattavissa);
    tyypit_.append( tietue );
    map_.insert(koodi, tietue);
}
