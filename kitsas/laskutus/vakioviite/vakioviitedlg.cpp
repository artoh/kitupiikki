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
#include "vakioviitedlg.h"
#include "ui_vakioviitedlg.h"
#include "db/kirjanpito.h"

#include <QRegularExpression>
#include <QPushButton>
#include <QSettings>
#include <QRegularExpressionValidator>

#include "validator/viitevalidator.h"
#include "vakioviitemodel.h"

VakioViiteDlg::VakioViiteDlg(VakioViiteModel *model, QWidget *parent) :
    QDialog(parent),
    model_(model),
    ui(new Ui::VakioViiteDlg)
{
    ui->setupUi(this);

    ui->tiliEdit->suodataTyypilla("C.*");
    connect( ui->buttonBox, &QDialogButtonBox::helpRequested, [] { kp()->ohje("/laskutus/vakioviite");} );
    ui->viiteEdit->setValidator(new QRegularExpressionValidator(QRegularExpression("^[1-9]\\d{3,6}")));

    ui->varoKuva->hide();
    ui->varoLabel->hide();

    bool onkoKohdennuksia = kp()->kohdennukset()->kohdennuksia();
    ui->kohdennusLabel->setVisible(onkoKohdennuksia);
    ui->kohdennusCombo->setVisible(onkoKohdennuksia);
}

VakioViiteDlg::~VakioViiteDlg()
{
    delete ui;
}

void VakioViiteDlg::muokkaa(const QVariantMap &map)
{    
    ui->viiteEdit->setText(map.value("viite").toString());
    ui->viiteEdit->setReadOnly(true);
    ui->numerointiGroup->setVisible(false);

    ui->otsikkoEdit->setText(map.value("otsikko").toString());
    ui->tiliEdit->valitseTiliNumerolla(map.value("tili").toInt());
    ui->kohdennusCombo->valitseKohdennus(map.value("kohdennus").toInt());
    exec();
}

void VakioViiteDlg::uusi()
{
    ui->kohdennusCombo->setCurrentIndex(
                ui->kohdennusCombo->findData(0, KohdennusModel::IdRooli));
    ui->tiliEdit->valitseTiliNumerolla(kp()->asetukset()->luku("OletusMyyntitili"));
    ui->viiteEdit->setText(model_->seuraava());

    ui->valitseRadio->setChecked( kp()->settings()->value("NumeroiVakioviiteItse").toBool() );
    ui->viiteEdit->setEnabled( ui->valitseRadio->isChecked() );


    connect( ui->viiteEdit, &QLineEdit::textChanged, this, &VakioViiteDlg::tarkasta );
    connect( ui->valitseRadio, &QRadioButton::toggled, ui->viiteEdit, &QLineEdit::setEnabled);
    connect( ui->valitseRadio, &QRadioButton::toggled, this, &VakioViiteDlg::tarkasta);

    exec();
}

void VakioViiteDlg::tarkasta()
{
    bool onkoviitejo = model_->onkoViitetta(ui->viiteEdit->text());

    bool kelpo =
            ui->seuraavaRadio->isChecked() || (
            ViiteValidator::kelpaako(ui->viiteEdit->text()) &&
            !onkoviitejo );

    bool varoita = ui->valitseRadio->isChecked() && onkoviitejo;
    ui->varoKuva->setVisible(varoita);
    ui->varoLabel->setVisible(varoita);

    ui->buttonBox->button(QDialogButtonBox::Ok)->setEnabled(kelpo);
}

void VakioViiteDlg::accept()
{
    QVariantMap map;

    map.insert("otsikko", ui->otsikkoEdit->text());
    map.insert("tili", ui->tiliEdit->valittuTilinumero());
    map.insert("kohdennus", ui->kohdennusCombo->kohdennus());

    KpKysely *kysely = ui->viiteEdit->isVisible() ? kpk(QString("/vakioviitteet/%1").arg(ui->viiteEdit->text()), KpKysely::PUT) :
                                                    kpk("/vakioviitteet", KpKysely::POST);
    connect( kysely, &KpKysely::vastaus, this, &VakioViiteDlg::tallennettu );
    kysely->kysy(map);

    if(ui->numerointiGroup->isVisible())
        kp()->settings()->setValue("NumeroiVakioviiteItse", ui->valitseRadio->isChecked());
}

void VakioViiteDlg::tallennettu()
{
    QDialog::accept();
}
