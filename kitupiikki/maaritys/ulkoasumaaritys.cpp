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

#include "ulkoasumaaritys.h"
#include "db/kirjanpito.h"

#include "ui_ulkoasumaaritys.h"

#include <QSettings>
#include <QApplication>

UlkoasuMaaritys::UlkoasuMaaritys() :
    MaaritysWidget(),
    ui(new Ui::Ulkoasu)
{
    ui->setupUi(this);

    for(int i=8; i < 20; i++) {
        ui->kokoCombo->addItem(QString("%1 pt").arg(i), i);
    }
    ui->kokoCombo->setCurrentIndex(4);

    connect(ui->oletusfontti, &QRadioButton::clicked, this, &UlkoasuMaaritys::asetaFontti);
    connect(ui->omafontti, &QRadioButton::clicked, this, &UlkoasuMaaritys::asetaFontti);
    connect(ui->fonttiCombo, &QFontComboBox::currentTextChanged, this, &UlkoasuMaaritys::asetaFontti);
    connect(ui->kokoCombo, &QComboBox::currentTextChanged, this, &UlkoasuMaaritys::asetaFontti);

}

UlkoasuMaaritys::~UlkoasuMaaritys()
{
    delete ui;
}

bool UlkoasuMaaritys::nollaa()
{

    QString fonttinimi = kp()->settings()->value("Fontti").toString();
    int koko = kp()->settings()->value("FonttiKoko").toInt();

    if( fonttinimi.isEmpty())
        ui->oletusfontti->setChecked(true);
    else {
        ui->omafontti->setChecked(true);
        ui->fonttiCombo->setCurrentFont(QFont(fonttinimi));
        ui->kokoCombo->setCurrentText(QString("%1 pt").arg(koko));
    }

    ui->fonttiCombo->setEnabled( !fonttinimi.isEmpty() );
    ui->kokoCombo->setEnabled( !fonttinimi.isEmpty());

    return true;
}

void UlkoasuMaaritys::asetaFontti()
{
    if( ui->oletusfontti->isChecked()) {
        kp()->settings()->remove("Fontti");
        qApp->setFont( oletusfontti__ );
    } else {
        QFont fontti = ui->fonttiCombo->currentFont();
        qApp->setFont( QFont( fontti.family(), ui->kokoCombo->currentData().toInt() ) );
        kp()->settings()->setValue("Fontti", fontti.family());
        kp()->settings()->setValue("FonttiKoko", ui->kokoCombo->currentData().toInt());
    }
}

QFont UlkoasuMaaritys::oletusfontti__ = QFont();
