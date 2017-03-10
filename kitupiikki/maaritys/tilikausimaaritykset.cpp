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

#include "tilikausimaaritykset.h"
#include "db/kirjanpito.h"

TilikausiMaaritykset::TilikausiMaaritykset()
{
    ui = new Ui::TilikausiMaaritykset;
    ui->setupUi(this);
}

TilikausiMaaritykset::~TilikausiMaaritykset()
{
    delete ui;
}

bool TilikausiMaaritykset::nollaa()
{
    ui->view->setModel( kp()->tilikaudet() );
    ui->view->resizeColumnsToContents();
    return true;
}
