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
#include "arkistohakemistodialogi.h"
#include "ui_arkistohakemistodialogi.h"

#include "db/kirjanpito.h"
#include "sqlite/sqlitemodel.h"

#include <QSettings>
#include <QFileDialog>
#include <QFileInfo>
#include <QRegularExpression>
#include <QRegularExpressionValidator>
#include <QPushButton>
#include <QMessageBox>

ArkistohakemistoDialogi::ArkistohakemistoDialogi(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::ArkistohakemistoDialogi)
{
    ui->setupUi(this);    

    SQLiteModel *sqlite = qobject_cast<SQLiteModel*>( kp()->yhteysModel() );
    if( sqlite ) {
        QFileInfo info(sqlite->tiedostopolku());
        QString nimi = info.fileName();
        if( nimi.endsWith(".kitsas")) {
            nimi.replace(".kitsas","-arkisto");
            ui->sijaintiEdit->setText( info.dir().absolutePath());
            ui->nimiEdit->setText(nimi);
        }
    } else {
        QString nimi = kp()->asetukset()->asetus("Nimi");
        nimi.replace(QRegularExpression("\\W",QRegularExpression::UseUnicodePropertiesOption),"");
        ui->sijaintiEdit->setText( QDir::homePath() );
        ui->nimiEdit->setText(nimi);
    }

    QString polku = kp()->settings()->value("arkistopolku/" + kp()->asetus("UID")).toString();
    if( !polku.isEmpty()) {
        QFileInfo info(polku);
        if( info.isDir()) {
            ui->sijaintiEdit->setText( info.dir().absolutePath() );
            ui->nimiEdit->setText( info.fileName());
        }
    }
    ui->nimiEdit->setValidator(new QRegularExpressionValidator(QRegularExpression("([A-Za-z0-9]|-){1,64}"), this));
    connect( ui->sijaintiEdit, &QLineEdit::textChanged, this, &ArkistohakemistoDialogi::tarkistaKelpo );
    connect( ui->nimiEdit, &QLineEdit::textChanged, this, &ArkistohakemistoDialogi::tarkistaKelpo);
}

ArkistohakemistoDialogi::~ArkistohakemistoDialogi()
{
    delete ui;
}

QString ArkistohakemistoDialogi::valitseArkistoHakemisto(QWidget *parent)
{
    ArkistohakemistoDialogi dlg(parent);
    if( dlg.exec() == Accepted)
        return dlg.polku();
    else
        return kp()->settings()->value("arkistopolku/" + kp()->asetus("UID")).toString();
}

void ArkistohakemistoDialogi::accept()
{
    QDir dir(ui->sijaintiEdit->text());
    dir.mkdir(ui->nimiEdit->text());
    if( dir.cd(ui->nimiEdit->text())) {
        polku_ = dir.absolutePath();
        kp()->settings()->setValue("arkistopolku/" + kp()->asetus("UID"), polku_);
        QDialog::accept();
    } else {
        QMessageBox::critical(this, tr("Virhe"),tr("Arkistohakemiston luominen epäonnistui."));
    }

}

void ArkistohakemistoDialogi::valitseHakemisto()
{
    QString hakemisto = QFileDialog::getExistingDirectory(nullptr, tr("Valitse hakemisto"),
                                                          ui->sijaintiEdit->text());
    if( !hakemisto.isEmpty())
        ui->sijaintiEdit->setText( hakemisto );
}

void ArkistohakemistoDialogi::tarkistaKelpo()
{
    ui->buttonBox->button(QDialogButtonBox::Ok)->setEnabled(
                !ui->sijaintiEdit->text().isEmpty() &&
                !ui->nimiEdit->text().isEmpty()
         );
}
