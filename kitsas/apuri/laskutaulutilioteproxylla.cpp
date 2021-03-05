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
#include "tiliote/tiliotemodel.h"

LaskuTauluTilioteProxylla::LaskuTauluTilioteProxylla(QObject *parent, TilioteModel *tiliote) :
    LaskuTauluModel (parent), tiliote_(tiliote)
{
    connect( tiliote, &VanhaTilioteModel::dataChanged, this, &LaskuTauluTilioteProxylla::paivitaSuoritukset);
    connect( tiliote, &VanhaTilioteModel::rowsInserted, this, &LaskuTauluTilioteProxylla::paivitaSuoritukset);
    connect( tiliote, &VanhaTilioteModel::rowsRemoved, this, &LaskuTauluTilioteProxylla::paivitaSuoritukset);
    connect( tiliote, &VanhaTilioteModel::modelReset, this, &LaskuTauluTilioteProxylla::paivitaSuoritukset);
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
        VanhaTilioteModel::Tilioterivi rivi = tiliote_->rivi(i);
        if( !rivi.era.isEmpty() && !rivi.harmaa) {
            double suoritus = ostoja_ ?
                        0.0 - rivi.euro :
                        rivi.euro;
            int eraid = rivi.era.value("id").toInt();
            suoritukset_.insert( eraid,
                                 suoritus + suoritukset_.value( eraid ));
        }
    }
    emit dataChanged( index(0, MAKSAMATTA),
                      index(rowCount()-1, MAKSAMATTA ));
}

void LaskuTauluTilioteProxylla::tietoSaapuu(QVariant *var)
{
    LaskuTauluModel::tietoSaapuu(var);
    paivitaSuoritukset();
}
