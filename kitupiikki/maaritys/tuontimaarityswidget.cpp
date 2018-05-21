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

#include "tuontimaarityswidget.h"

#include "tuonti/tuonti.h"

TuontiMaaritysWidget::TuontiMaaritysWidget()
    : MaaritysWidget(),
      ui( new Ui::TuontiMaaritys)
{
    ui->setupUi( this );

    ui->tositelajiCombo->setModel( kp()->tositelajit());

    ui->perusteCombo->addItem(QIcon(":/pic/suorite.png"), tr("Suoriteperusteinen"), Tuonti::SUORITEPERUSTEINEN);
    ui->perusteCombo->addItem(QIcon(":/pic/kirje.png"), tr("Laskutusperusteinen"), Tuonti::LASKUPERUSTEINEN);
    ui->perusteCombo->addItem(QIcon(":/pic/euro.png"), tr("Maksuperusteinen"), Tuonti::MAKSUPERUSTEINEN);

    ui->velkatiliEdit->suodataTyypilla("BO");

    connect( ui->ostolaskuGroup, SIGNAL(clicked(bool)), this, SLOT(ilmoitaMuokattu()));
    connect( ui->perusteCombo, SIGNAL(currentIndexChanged(int)), this, SLOT(ilmoitaMuokattu()));
    connect( ui->tositelajiCombo, SIGNAL(currentIndexChanged(int)), this, SLOT(ilmoitaMuokattu()));
    connect( ui->velkatiliEdit, SIGNAL(textChanged(QString)), this, SLOT(ilmoitaMuokattu()));
    connect( ui->tilioteRadio, SIGNAL(clicked(bool)), this, SLOT(ilmoitaMuokattu()));

}

TuontiMaaritysWidget::~TuontiMaaritysWidget()
{
    delete ui;
}

bool TuontiMaaritysWidget::nollaa()
{
    int peruste = kp()->asetukset()->luku("TuontiOstolaskuPeruste");

    ui->ostolaskuGroup->setChecked( peruste );
    ui->tositelajiCombo->setCurrentIndex( ui->tositelajiCombo->findData( kp()->asetukset()->luku("TuontiOstolaskuTositelaji") ));

    if( peruste )
        ui->perusteCombo->setCurrentIndex( ui->perusteCombo->findData( peruste ));

    ui->velkatiliEdit->valitseTiliNumerolla( kp()->asetukset()->luku("TuontiOstolaskuTili") );

    ui->tilioteRadio->setChecked( kp()->asetukset()->onko("TuontiTiliote") );

    return true;
}

bool TuontiMaaritysWidget::tallenna()
{
    int peruste = ui->perusteCombo->currentData().toInt();
    if( !ui->ostolaskuGroup->isChecked())
        peruste = 0;

    kp()->asetukset()->aseta("TuontiOstolaskuPeruste", peruste);
    kp()->asetukset()->aseta("TuontiOstolaskuTositelaji", ui->tositelajiCombo->currentData().toInt());
    kp()->asetukset()->aseta("TuontiOstolaskuTili", ui->velkatiliEdit->valittuTilinumero());
    kp()->asetukset()->aseta("TuontiTiliote", ui->tilioteRadio->isChecked());

    return true;
}

bool TuontiMaaritysWidget::onkoMuokattu()
{
    int peruste = ui->perusteCombo->currentData().toInt();
    if( !ui->ostolaskuGroup->isChecked())
        peruste = 0;

    return peruste != kp()->asetukset()->luku("TuontiOstolaskuPeruste") ||
           ui->tositelajiCombo->currentData().toInt() != kp()->asetukset()->luku("TuontiOstolaskuTositelaji") ||
           ui->velkatiliEdit->valittuTilinumero() != kp()->asetukset()->luku("TuontiOstolaskuTili") ||
            ui->tilioteRadio->isChecked() != kp()->asetukset()->onko("TuontiTiliote");
}

void TuontiMaaritysWidget::ilmoitaMuokattu()
{
    emit tallennaKaytossa( onkoMuokattu());
}
