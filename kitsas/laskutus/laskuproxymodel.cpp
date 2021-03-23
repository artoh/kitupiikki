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
#include "laskuproxymodel.h"

#include "model/laskutaulumodel.h"
#include "db/tositetyyppimodel.h"

#include "viitenumero.h"

LaskuProxyModel::LaskuProxyModel(QObject *parent) :
    QSortFilterProxyModel(parent)
{
    setSortRole(Qt::EditRole);
}

void LaskuProxyModel::suodataNumerolla(const QString &numero)
{
    numero_ = numero;
    invalidateFilter();
}

void LaskuProxyModel::suodataTekstilla(const QString &teksti)
{
    teksti_ = teksti;
    kumppani_ = 0;
    invalidateFilter();
}

void LaskuProxyModel::suodataViittella(const QString &viite)
{
    viite_ = viite;
    invalidateFilter();
}

void LaskuProxyModel::suodataLaskut(bool vainLaskut)
{
    vainLaskut_ = vainLaskut;
    invalidateFilter();
}

void LaskuProxyModel::suodataKumppani(int kumppani)
{
    kumppani_ = kumppani;
    invalidateFilter();
}

void LaskuProxyModel::suodataViiteTyypilla(int viitetyyppi)
{
    viitetyyppi_ = viitetyyppi;
    invalidateFilter();
}

void LaskuProxyModel::nollaaSuodatus()
{
    numero_.clear();
    viite_.clear();
    kumppani_ = 0;
    viitetyyppi_ = 0;
    invalidateFilter();
}

bool LaskuProxyModel::filterAcceptsRow(int source_row, const QModelIndex &source_parent) const
{
    QModelIndex idx = sourceModel()->index(source_row, 0, source_parent);

    if( !teksti_.isEmpty()
            && !idx.data(LaskuTauluModel::AsiakasToimittajaNimiRooli).toString().contains(teksti_, Qt::CaseInsensitive)
            && !idx.data(LaskuTauluModel::OtsikkoRooli).toString().contains(teksti_, Qt::CaseInsensitive))
        return false;

    if( kumppani_ && idx.data(LaskuTauluModel::AsiakasToimittajaIdRooli).toInt() != kumppani_)
        return false;

    if( !numero_.isEmpty() &&
            !idx.data(LaskuTauluModel::ViiteRooli).toString().contains(numero_, Qt::CaseInsensitive)    &&
            !idx.data(LaskuTauluModel::NumeroRooli).toString().contains(numero_, Qt::CaseInsensitive))
        return false;

    if( vainLaskut_ && !idx.data(LaskuTauluModel::OstoLaskutTieto).toBool()) {
        int tyyppi = idx.data(LaskuTauluModel::TyyppiRooli).toInt();
        if( tyyppi < TositeTyyppi::MYYNTILASKU || tyyppi > TositeTyyppi::MAKSUMUISTUTUS)
            return false;
    }

    if( !viite_.isEmpty() &&
        idx.data(LaskuTauluModel::ViiteRooli).toString() != viite_ )
        return false;

    if( viitetyyppi_ ) {
        ViiteNumero viite( idx.data(LaskuTauluModel::ViiteRooli).toString() );
        if( viite.tyyppi() != viitetyyppi_ )
            return false;
    }

    return true;
}
