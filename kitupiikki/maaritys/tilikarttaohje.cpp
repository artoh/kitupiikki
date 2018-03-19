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
#include "tools/devtool.h"

TilikarttaOhje::TilikarttaOhje() :
    ui(new Ui::TilikarttaOhje)
{
    ui->setupUi(this);
    ui->ohjeEditori->setVisible(false);

    connect( ui->muokkausNappi, SIGNAL(clicked(bool)),
             this, SIGNAL(tallennaKaytossa(bool)));
    connect( ui->devtoolButton, SIGNAL(clicked(bool)),
             this, SLOT(devtool()));

}

bool TilikarttaOhje::nollaa()
{
    ui->nimiLabel->setText( kp()->asetukset()->asetus("TilikarttaNimi") );
    ui->tekijaLabel->setText( kp()->asetukset()->asetus("TilikarttaTekija"));
    ui->pvmLabel->setText( kp()->asetukset()->pvm("TilikarttaPvm").toString("dd.MM.yyyy") );
    ui->ohjeBrowser->setHtml( kp()->asetukset()->asetus("TilikarttaOhje"));
    ui->ohjeEditori->setText(kp()->asetukset()->asetus("TilikarttaOhje"));

    ui->muokkausNappi->setVisible( kp()->asetukset()->onko("NaytaEdistyneet") );
    ui->devtoolButton->setVisible( kp()->asetukset()->onko("NaytaEdistyneet") );

    return true;
}

bool TilikarttaOhje::tallenna()
{
    kp()->asetukset()->aseta("TilikarttaOhje", ui->ohjeEditori->toHtml());
    ui->ohjeBrowser->setHtml( kp()->asetukset()->asetus("TilikarttaOhje"));
    ui->muokkausNappi->setChecked(false);
    ui->ohjeEditori->setVisible(false);

    return true;
}

bool TilikarttaOhje::onkoMuokattu()
{
    return ui->muokkausNappi->isChecked() &&  ui->ohjeEditori->toHtml() != kp()->asetukset()->asetus("TilikarttaOhje");
}

void TilikarttaOhje::devtool()
{
    DevTool *tool = new DevTool();
    tool->setAttribute(Qt::WA_DeleteOnClose);
    tool->show();

}
