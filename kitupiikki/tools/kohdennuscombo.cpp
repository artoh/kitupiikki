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
#include "kohdennuscombo.h"
#include "kirjaus/kohdennusproxymodel.h"
#include "db/kirjanpito.h"

KohdennusCombo::KohdennusCombo(QWidget *parent)
    : QComboBox(parent),
      proxy_( new KohdennusProxyModel(this))
{
    proxy_->setSourceModel( kp()->kohdennukset() );
    setModel( proxy_ );

    setModelColumn( KohdennusModel::NIMI);
    connect( this, SIGNAL(currentIndexChanged(int)), this, SLOT(vaihtuu()) );

    setCurrentIndex( findData(0, KohdennusModel::IdRooli) );
}

int KohdennusCombo::kohdennus() const
{
    return currentData(KohdennusModel::IdRooli).toInt();
}

void KohdennusCombo::valitseKohdennus(int kohdennus)
{
    proxy_->asetaKohdennus(kohdennus);
    setCurrentIndex( findData(kohdennus, KohdennusModel::IdRooli) );
}

void KohdennusCombo::suodataPaivalla(const QDate &pvm)
{
    proxy_->asetaPaiva(pvm);
}

void KohdennusCombo::suodataValilla(const QDate &alkaa, const QDate &paattyy)
{
    proxy_->asetaVali(alkaa, paattyy);
}

void KohdennusCombo::vaihtuu()
{
    emit kohdennusValittu( kohdennus() );
}
