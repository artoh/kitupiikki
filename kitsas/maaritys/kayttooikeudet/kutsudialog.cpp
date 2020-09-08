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
#include "kutsudialog.h"
#include "ui_kutsudialog.h"

#include "db/kirjanpito.h"
#include "pilvi/pilvimodel.h"

#include <QRegularExpression>
#include <QPushButton>

KutsuDialog::KutsuDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::KutsuDialog)
{
    ui->setupUi(this);

    connect(ui->emailEdit, &QLineEdit::textChanged, this, &KutsuDialog::check);
    connect(ui->nimiEdit, &QLineEdit::textChanged, this, &KutsuDialog::check);

    check();
}

KutsuDialog::~KutsuDialog()
{
    delete ui;
}

void KutsuDialog::check()
{
    QRegularExpression emailRe(R"(^.*@.*\.\w+$)");

    ui->buttonBox->button(QDialogButtonBox::Ok)->setEnabled(
                emailRe.match(ui->emailEdit->text()).hasMatch() &&
                ui->nimiEdit->text().length() > 3);

}

void KutsuDialog::kutsuttu()
{
    QDialog::accept();
}

void KutsuDialog::accept()
{
    KpKysely* kysely = kpk(QString("%1/invite")
                           .arg(kp()->pilvi()->pilviLoginOsoite()),
                           KpKysely::POST);
    QVariantMap map;
    map.insert("email", ui->emailEdit->text());
    map.insert("name", ui->nimiEdit->text());

    connect(kysely, &KpKysely::vastaus, this, &KutsuDialog::kutsuttu);
    kysely->kysy(map);
}

QString KutsuDialog::kutsu(const QString &email)
{
    KutsuDialog dlg;
    dlg.ui->emailEdit->setText(email);
    if(dlg.exec() == QDialog::Accepted) {
        return dlg.ui->emailEdit->text();
    } else {
        return QString();
    }
}
