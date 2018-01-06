/*
   Copyright (C) 2018 Arto Hyvättinen

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

#include "tilikarttaohje.h"
#include "db/kirjanpito.h"
#include "ui_tilikarttaohje.h"

TilikarttaOhje::TilikarttaOhje() :
    ui(new Ui::TilikarttaOhje)
{
    ui->setupUi(this);
}

bool TilikarttaOhje::nollaa()
{
    ui->nimiLabel->setText( kp()->asetukset()->asetus("TilikarttaNimi") );
    ui->tekijaLabel->setText( kp()->asetukset()->asetus("TilikarttaTekija"));
    ui->pvmLabel->setText( kp()->asetukset()->pvm("TilikarttaPvm").toString(Qt::SystemLocaleShortDate) );
    ui->ohjeBrowser->setHtml( kp()->asetukset()->asetus("TilikarttaOhje"));

    return true;
}
