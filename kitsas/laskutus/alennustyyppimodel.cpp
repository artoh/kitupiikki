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
#include "alennustyyppimodel.h"

AlennusTyyppiModel::AlennusTyyppiModel(QObject *parent)
    : QAbstractListModel(parent)
{
    // UNCL5189
    // Verkkolaskussa käytettävät alennuksen syykoodit
    // Ellei muuta, käytetään 95 - Alennus
    lisaa(95, tr("Alennus"));
    lisaa(100, "Erikoistarjous");
    lisaa(102,tr("Kiinteänpituinen kausi"));
    lisaa(103, tr("Tilapäinen"));
    lisaa(104, tr("Vakio"));
    lisaa(105, tr("Vuosittainen myynti"));
    lisaa(41, tr("Hyvitys aikataulua edellä olevasta työstä"));
    lisaa(42, tr("Muu hyvitys"));
    lisaa(60, tr("Valmistajan kuluttaja-alennus"));
    lisaa(62, tr("Asiakkaan sotilaallinen asema"));
    lisaa(63, tr("Työonnettomuus"));
    lisaa(64, tr("Erikoissopimus"));
    lisaa(65, tr("Alennus tuotantovirheestä"));
    lisaa(66, tr("Uuden markkinan alennus"));
    lisaa(67, tr("Näytealennus"));
    lisaa(68, tr("Loppuerän alennus"));
    lisaa(70, tr("Toimituslausekealennus"));
    lisaa(71, tr("Myyntikynnysalennus"));
    lisaa(88, tr("Materiaalin ylijäämä/vähentäminen"));
}

int AlennusTyyppiModel::rowCount(const QModelIndex &parent) const
{
    if (parent.isValid())
        return 0;

    return alennusTyypit_.count();
}


QVariant AlennusTyyppiModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid())
        return QVariant();

    const AlennusTyyppi& tyyppi = alennusTyypit_.at(index.row());

    switch (role) {
    case Qt::DisplayRole: return tyyppi.nimi();
    case KoodiRooli: return tyyppi.koodi();
    default:
        return QVariant();
    }
}

void AlennusTyyppiModel::lisaa(int koodi, const QString &nimi)
{
    alennusTyypit_.append(AlennusTyyppi(koodi, nimi));
}

AlennusTyyppiModel::AlennusTyyppi::AlennusTyyppi()
{

}

AlennusTyyppiModel::AlennusTyyppi::AlennusTyyppi(int koodi, const QString &nimi)
    : koodi_(koodi), nimi_(nimi)
{

}
