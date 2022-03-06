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
#include "kieli/kielet.h"
#include "saldodock/saldodock.h"
#include <QMessageBox>

UlkoasuMaaritys::UlkoasuMaaritys() :
    MaaritysWidget(),
    ui(new Ui::Ulkoasu)
{
    ui->setupUi(this);

    for(int i=8; i < 21; i++) {
        ui->kokoCombo->addItem(QString("%1 pt").arg(i), i);
    }

    ui->kokoCombo->setCurrentIndex(4);
    ui->fonttiCombo->setCurrentFont(QFont("FreeSans"));

    connect(ui->oletusfontti, &QRadioButton::clicked, this, &UlkoasuMaaritys::asetaFontti);
    connect(ui->omafontti, &QRadioButton::clicked, this, &UlkoasuMaaritys::asetaFontti);
    connect(ui->fonttiCombo, &QFontComboBox::currentFontChanged, this, &UlkoasuMaaritys::asetaFontti);
    connect(ui->kokoCombo, &QComboBox::currentTextChanged, this, &UlkoasuMaaritys::asetaFontti);
    connect(ui->saldotCheck, &QCheckBox::clicked, this, &UlkoasuMaaritys::naytaSaldot);

    connect( ui->fiKieli, &QRadioButton::clicked, this, &UlkoasuMaaritys::vaihdaKieli);
    connect( ui->svKieli, &QRadioButton::clicked, this, &UlkoasuMaaritys::vaihdaKieli);
    connect( ui->tilikarttaKieli, &KieliCombo::currentTextChanged, this, &UlkoasuMaaritys::vaihdaTilikarttaKieli);
}

UlkoasuMaaritys::~UlkoasuMaaritys()
{
    delete ui;
}

bool UlkoasuMaaritys::nollaa()
{

    QString fonttinimi = kp()->settings()->value("Fontti").toString();
    int koko = kp()->settings()->value("FonttiKoko").toInt();

    if( fonttinimi.isEmpty()) {
        ui->oletusfontti->setChecked(true);

    }
    else {
        ui->omafontti->setChecked(true);
        ui->fonttiCombo->setCurrentFont(QFont(fonttinimi));
        ui->kokoCombo->setCurrentText(QString("%1 pt").arg(koko));
    }

    ui->fonttiCombo->setEnabled( !fonttinimi.isEmpty() );
    ui->kokoCombo->setEnabled( !fonttinimi.isEmpty());

    ui->fiKieli->setChecked( Kielet::instanssi()->uiKieli() == "fi" );
    ui->svKieli->setChecked( Kielet::instanssi()->uiKieli() == "sv" );

    ui->karttakieliGroup->setVisible( kp()->yhteysModel() );
    ui->tilikarttaKieli->valitse( Kielet::instanssi()->nykyinen() );

    ui->saldotCheck->setChecked( kp()->settings()->value("SaldoDock").toBool() );

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

void UlkoasuMaaritys::naytaSaldot(bool naytetaanko)
{
    kp()->settings()->setValue("SaldoDock", naytetaanko);
    SaldoDock::dock()->alusta();
}

void UlkoasuMaaritys::vaihdaKieli()
{
    if( ui->svKieli->isChecked()) {
        Kielet::instanssi()->valitseUiKieli("sv");
        ui->tilikarttaKieli->valitse("sv");
    } else {
        Kielet::instanssi()->valitseUiKieli("fi");
        ui->tilikarttaKieli->valitse("fi");
    }

    QMessageBox::information(this, tr("Kieli vaihdettu"),
                             tr("Käynnistä kielen vaihtamisen jälkeen ohjelma uudelleen, "
                                "jotta valitsemasi kieli tulee käyttöön kaikissa näkymissä."));
}

void UlkoasuMaaritys::vaihdaTilikarttaKieli()
{
    Kielet::instanssi()->valitseKieli( ui->tilikarttaKieli->kieli() );
    QString kieliAvain = kp()->asetukset()->uid() + "/kieli";
    if( ui->tilikarttaKieli->kieli() == Kielet::instanssi()->uiKieli())
        kp()->settings()->remove(kieliAvain);
    else
        kp()->settings()->setValue(kieliAvain, ui->tilikarttaKieli->kieli());
}

QFont UlkoasuMaaritys::oletusfontti__ = QFont();
