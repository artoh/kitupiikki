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
#include "tilivalintadialogifiltteri.h"
#include "db/tilimodel.h"

TilivalintaDialogiFiltteri::TilivalintaDialogiFiltteri(QObject *parent)
    : QSortFilterProxyModel(parent)
{

}

void TilivalintaDialogiFiltteri::suodataTyypilla(const QString &tilityyppi)
{
    tyyppiSuodatus_ = QRegularExpression(tilityyppi);
    doFiltering();
    invalidate();
}

void TilivalintaDialogiFiltteri::suodataTekstilla(const QString &teksti)
{
    if( teksti.toInt()) {
        numeroSuodatus_ = teksti;
        nimiSuodatus_.clear();
    } else {
        numeroSuodatus_.clear();
        nimiSuodatus_ = teksti;
    }
    doFiltering();
    invalidate();
}

void TilivalintaDialogiFiltteri::suodataTilalla(int tila)
{
    tilaSuodatus_ = tila;
    doFiltering();
    invalidate();
}

void TilivalintaDialogiFiltteri::naytaOtsikot(bool nayta)
{
    otsikot_ = nayta;
    doFiltering();
    invalidate();
}

void TilivalintaDialogiFiltteri::doFiltering()
{

    int tasolle = 0;
    suodatusSet_.clear();

    for(int i = sourceModel()->rowCount()-1; i >= 0; i--) {
        QModelIndex sourceIndex = sourceModel()->index(i, 0);

        int otsikkotaso = sourceIndex.data(TiliModel::OtsikkotasoRooli).toInt();
        if( otsikkotaso ) {
            if( otsikot_ && tasolle > otsikkotaso) {
                tasolle = otsikkotaso;
                suodatusSet_.insert(i);
            }
        } else {
            if( !numeroSuodatus_.isEmpty() && !sourceIndex.data(TiliModel::NroRooli).toString().startsWith(numeroSuodatus_))
                continue;
            if( !nimiSuodatus_.isEmpty() && !sourceIndex.data(TiliModel::NimiRooli).toString().contains(nimiSuodatus_, Qt::CaseInsensitive))
                continue;

            int tila = sourceIndex.data(TiliModel::TilaRooli).toInt();
            if( tilaSuodatus_ == KAYTOSSA && tila < Tili::TILI_KAYTOSSA)
                continue;
            if( tilaSuodatus_ == SUOSIKIT && tila < Tili::TILI_SUOSIKKI)
                continue;
            if( tilaSuodatus_ == KIRJATTU && qAbs(sourceIndex.data(TiliModel::SaldoRooli).toDouble()) < 1e-5)
                continue;

            if( tyyppiSuodatus_.isValid() && !tyyppiSuodatus_.match(sourceIndex.data(TiliModel::TyyppiRooli).toString()).hasMatch())
                continue;

            tasolle = 255;
            suodatusSet_.insert(i);
        }
    }

    invalidateFilter();

}

bool TilivalintaDialogiFiltteri::filterAcceptsRow(int source_row, const QModelIndex &/*source_parent*/) const
{
    return suodatusSet_.contains(source_row);
}
