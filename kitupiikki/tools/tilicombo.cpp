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
#include "tilicombo.h"
#include "db/kirjanpito.h"

#include <QSortFilterProxyModel>
#include <QDebug>

TiliCombo::TiliCombo(QWidget *parent) :
    QComboBox (parent)
{
    proxyTila_ = new QSortFilterProxyModel(this);
    proxyTila_->setSourceModel( kp()->tilit() );
    proxyTila_->setFilterRole(TiliModel::TilaRooli);
    proxyTila_->setFilterRegularExpression("[12]");

    proxyTyyppi_ = new QSortFilterProxyModel(this);
    proxyTyyppi_->setSourceModel( proxyTila_);
    proxyTyyppi_->setFilterRole(TiliModel::TyyppiRooli);
    proxyTyyppi_->setFilterRegularExpression("[ABCD].*");

    setModel( proxyTyyppi_ );

    connect( this, &TiliCombo::currentTextChanged, this, &TiliCombo::vaihtui);
    connect( proxyTyyppi_, &QSortFilterProxyModel::modelReset, this, &TiliCombo::valitseEka);
}

void TiliCombo::suodataTyypilla(const QString &regexp)
{
    proxyTyyppi_->setFilterRole(TiliModel::TyyppiRooli);
    proxyTyyppi_->setFilterRegularExpression(regexp);
    if( currentIndex() < 0 )
        setCurrentIndex(0);
}

void TiliCombo::suodataMaksutapa(const QString &regexp)
{
    proxyTyyppi_->setFilterRole(TiliModel::MaksutapaRooli);
    proxyTyyppi_->setFilterRegExp(regexp);
    if( currentIndex() < 0 )
        setCurrentIndex(0);
}

int TiliCombo::valittuTilinumero() const
{
    return currentData(TiliModel::NroRooli).toInt();
}

void TiliCombo::valitseTili(int tilinumero)
{
    setCurrentIndex( findData(tilinumero, TiliModel::NroRooli) );
}

void TiliCombo::vaihtui()
{
    emit tiliValittu( valittuTilinumero() );
}

void TiliCombo::valitseEka()
{
    setCurrentIndex(0);
}
