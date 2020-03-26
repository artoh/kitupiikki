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
#include "liitemaaritys.h"

#include "ui_liitemaaritys.h"

#include "db/kirjanpito.h"
#include "pilvi/pilvimodel.h"
#include <QSettings>

LiiteMaaritys::LiiteMaaritys() :
    MaaritysWidget(nullptr),
    ui(new Ui::LiiteMaaritys)
{
    ui->setupUi(this);

    connect( ui->ocrCheck, &QCheckBox::clicked, this, &LiiteMaaritys::ilmoitaMuokattu );
    connect( ui->mvCheck, &QCheckBox::clicked, this, &LiiteMaaritys::ilmoitaMuokattu);
    connect( ui->kokoScroll, &QSlider::valueChanged, this, &LiiteMaaritys::ilmoitaMuokattu );
    connect( ui->laatuScroll, &QSlider::valueChanged, this, &LiiteMaaritys::ilmoitaMuokattu );
}

LiiteMaaritys::~LiiteMaaritys()
{
    delete ui;
}

bool LiiteMaaritys::nollaa()
{
    ui->ocrCheck->setChecked( kp()->settings()->value("OCR", false).toBool() );
    ui->ocrCheck->setEnabled( kp()->pilvi()->plan() ||
                              qobject_cast<PilviModel*>(kp()->yhteysModel()) );
    ui->tilaajilleLabel->setVisible( !ui->ocrCheck->isEnabled() );
    ui->mvCheck->setChecked( kp()->settings()->value("KuvaMustavalko",false).toBool());
    ui->kokoScroll->setValue( kp()->settings()->value("KuvaKoko",2048).toInt());
    ui->laatuScroll->setValue( kp()->settings()->value("KuvaLaatu", 40).toInt());
    return true;
}

bool LiiteMaaritys::tallenna()
{
    kp()->settings()->setValue("OCR", ui->ocrCheck->isChecked());
    kp()->settings()->setValue("KuvaMustavalko", ui->mvCheck->isChecked());
    kp()->settings()->setValue("KuvaKoko", ui->kokoScroll->value());
    kp()->settings()->setValue("KuvaLaatu", ui->laatuScroll->value());
    return true;
}

bool LiiteMaaritys::onkoMuokattu()
{
    return  ui->ocrCheck->isChecked() != kp()->settings()->value("OCR",false).toBool() ||
            ui->mvCheck->isChecked() != kp()->settings()->value("KuvaMustavalko",false).toBool() ||
            ui->kokoScroll->value() != kp()->settings()->value("KuvaKoko", 2048).toInt() ||
            ui->laatuScroll->value() != kp()->settings()->value("KuvaLaatu",40).toInt();
}

void LiiteMaaritys::ilmoitaMuokattu()
{
    emit tallennaKaytossa(onkoMuokattu());
}
