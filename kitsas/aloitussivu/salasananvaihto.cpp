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
#include "salasananvaihto.h"
#include "ui_salasananvaihto.h"
#include "db/kirjanpito.h"
#include "pilvi/pilvimodel.h"
#include "pilvi/pilvikysely.h"

#include <QPushButton>
#include <QRegularExpression>
#include <QMessageBox>

Salasananvaihto::Salasananvaihto(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::Salasananvaihto)
{
    ui->setupUi(this);
    check();
    connect(ui->vanha, &QLineEdit::textEdited, this, &Salasananvaihto::check);
    connect(ui->uusi1, &QLineEdit::textEdited, this, &Salasananvaihto::check);
    connect(ui->uusi2, &QLineEdit::textEdited, this, &Salasananvaihto::check);
}

Salasananvaihto::~Salasananvaihto()
{
    delete ui;
}

void Salasananvaihto::accept()
{
    PilviKysely* kysely = new PilviKysely(kp()->pilvi(), KpKysely::POST, kp()->pilvi()->pilviLoginOsoite() + "/password");
    QVariantMap map;
    map.insert("oldpassword", ui->vanha->text());
    map.insert("newpassword", ui->uusi1->text());
    map.insert("invalidate", ui->invalisoi->isChecked());

    connect(kysely, &KpKysely::vastaus, this, &Salasananvaihto::vaihdettu);
    connect(kysely, &KpKysely::virhe, this, &Salasananvaihto::virhe);

    kysely->kysy(map);

}

void Salasananvaihto::check()
{
    ui->buttonBox->button(QDialogButtonBox::Ok)->setEnabled(false);

    QString uusi = ui->uusi1->text();

    if( ui->vanha->text().length() < 5) {
        ui->status->setText(tr("Kirjoita vanha salasana"));
    } else if(uusi.length() < 10) {
        ui->status->setText(tr("Uuden salasanan on oltava vähintään 10 merkkiä pitkä."));
    } else if(!uusi.contains(QRegularExpression("[A-Z]")) ||
              !uusi.contains(QRegularExpression("[a-z]")) ||
              !uusi.contains(QRegularExpression("[0-9]"))) {
        ui->status->setText(tr("Uudessa salasanassa on oltava pieniä ja isoja kirjaimia sekä numeroita."));
    } else if(uusi != ui->uusi2->text()) {
        ui->status->setText(tr("Salasanat eivät täsmää"));
    } else {
        ui->status->setText("");
        ui->buttonBox->button(QDialogButtonBox::Ok)->setEnabled(true);
    }

}

void Salasananvaihto::virhe()
{
    QMessageBox::critical(this, tr("Salasanan vaihto epäonnistui"), tr("Antamasi nykyinen salasana on väärin."));
}

void Salasananvaihto::vaihdettu()
{
    QMessageBox::information(this, tr("Salasana vaihdettu"), tr("Salasanasi on nyt vaihdettu."));
    QDialog::accept();

}
