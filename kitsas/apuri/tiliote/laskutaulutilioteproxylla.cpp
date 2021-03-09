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
#include "laskutaulutilioteproxylla.h"
#include "tiliotemodel.h"

LaskuTauluTilioteProxylla::LaskuTauluTilioteProxylla(QObject *parent, TilioteModel *tiliote) :
    LaskuTauluModel (parent), tiliote_(tiliote)
{
    connect( tiliote, &TilioteModel::dataChanged, this, &LaskuTauluTilioteProxylla::paivitaSuoritukset);
    connect( tiliote, &TilioteModel::rowsInserted, this, &LaskuTauluTilioteProxylla::paivitaSuoritukset);
    connect( tiliote, &TilioteModel::rowsRemoved, this, &LaskuTauluTilioteProxylla::paivitaSuoritukset);
    connect( tiliote, &TilioteModel::modelReset, this, &LaskuTauluTilioteProxylla::paivitaSuoritukset);
}

QVariant LaskuTauluTilioteProxylla::data(const QModelIndex &index, int role) const
{
    if( ( role == Qt::DisplayRole || role == Qt::EditRole) && index.column() == MAKSAMATTA)  {
        double avoinna = LaskuTauluModel::data(index, Qt::EditRole).toDouble();
        avoinna -= suoritukset_.value( LaskuTauluModel::data(index, EraIdRooli).toInt() );
        if( role != Qt::DisplayRole)
            return avoinna;
        else if( avoinna > 1e-5)
            return QString("%L1 €").arg( avoinna ,0,'f',2);
        else
            return QVariant();  // Nollalle tyhjää
    }

    return LaskuTauluModel::data(index, role);
}

void LaskuTauluTilioteProxylla::paivitaSuoritukset()
{
    // Haetaan avoimelta tiliotteelta suoritukset

    suoritukset_.clear();
    for(int i=0; i < tiliote_->rowCount(); i++) {
        const QModelIndex& index = tiliote_->index(i, TilioteRivi::EURO);
        const int eraId = index.data(TilioteRivi::EraIdRooli).toInt();
        const double euro = index.data(TilioteRivi::EuroRooli).toInt();

        double suoritus  = ostoja_ ? 0.0 - euro : euro;
        suoritukset_.insert( eraId, suoritus + suoritukset_.value(eraId));
    }
    emit dataChanged( index(0, MAKSAMATTA),
                      index(rowCount()-1, MAKSAMATTA ));
}

void LaskuTauluTilioteProxylla::tietoSaapuu(QVariant *var)
{
    LaskuTauluModel::tietoSaapuu(var);
    paivitaSuoritukset();
}
