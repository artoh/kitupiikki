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
#include "raportinmuokkausdialogi.h"
#include "ui_raportinmuokkausdialogi.h"
#include "db/kielikentta.h"

#include <QRegularExpression>
#include <QRegularExpressionValidator>
#include <QPushButton>

#include <QDebug>

RaportinmuokkausDialogi::RaportinmuokkausDialogi(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::RaportinmuokkausDialogi)
{
    ui->setupUi(this);
    ui->tilitEdit->setValidator(new QRegularExpressionValidator(
           QRegularExpression("(\\d{1,8}([.]{1,2}\\d{1,8})?[+-]?[,\\s]?)*"),this));
}

RaportinmuokkausDialogi::~RaportinmuokkausDialogi()
{
    delete ui;
}



QVariantMap RaportinmuokkausDialogi::muokkaa(const QVariantMap &data)
{
    RaportinmuokkausDialogi dlg;
    dlg.lataa(data);

    if( dlg.exec() == Accepted )
        return dlg.data();

    return data;
}

QVariantMap RaportinmuokkausDialogi::uusi()
{
    RaportinmuokkausDialogi dlg;
    dlg.alustaKielet();

    if( dlg.exec() == Accepted )
        return dlg.data();

    return QVariantMap();
}

void RaportinmuokkausDialogi::lataa(const QVariantMap &data)
{
    KieliKentta kk;
    QMapIterator<QString,QVariant> iter(data);
    while( iter.hasNext()) {
        iter.next();        
        if( iter.key().at(0).isLower())
            kk.aseta(iter.value().toString(), iter.key());
    }
    kk.alustaListWidget(ui->nimike);

    QString kaava = data.value("L").toString();
    QString muoto = data.value("M").toString();

    if( kaava.isEmpty() || kaava.contains('H')  || kaava.contains('h'))
        ui->otsikkoRadio->setChecked(true);
    else if( kaava.contains("=") && !kaava.contains("=="))
        ui->valisummaRadio->setChecked(true);
    else
        ui->summaRadio->setChecked(true);

    ui->tyhjaCheck->setChecked( kaava.isEmpty() || kaava.contains('H') || kaava.contains('S') );
    ui->laskevalisummaanCheck->setChecked( ui->summaRadio->isChecked() && !kaava.contains("==") );
    ui->sisennysSpin->setValue(data.value("S").toInt());
    ui->valiCheck->setChecked(data.value("V").toInt());
    ui->lihavoiCheck->setChecked(muoto.contains("bold"));
    ui->erittelyCheck->setChecked(kaava.contains('*'));

    kaava.remove(QRegularExpression("[*hHsS=]"));
    ui->tilitEdit->setText(kaava.simplified());
}

void RaportinmuokkausDialogi::alustaKielet()
{
    KieliKentta kk;
    kk.alustaListWidget(ui->nimike);
}

QVariantMap RaportinmuokkausDialogi::data() const
{
    KieliKentta kk;
    kk.lataa(ui->nimike);
    QVariantMap map(kk.map());

    if( ui->sisennysSpin->value())
        map.insert("S", ui->sisennysSpin->value());
    if( ui->valiCheck->isChecked())
        map.insert("V",1);
    if( ui->lihavoiCheck->isChecked())
        map.insert("M","bold");

    QString kaava = ui->tilitEdit->text();

    if( ui->summaRadio->isChecked() && ui->tyhjaCheck->isChecked()) {
        kaava.append(" S");

    } else if( ui->otsikkoRadio->isChecked() && !kaava.isEmpty()) {
        if( ui->tyhjaCheck->isChecked())
            kaava.append("H");
        else
            kaava.append("h");
    } else if( ui->valisummaRadio->isChecked())
        kaava.append(" =");
    if( ui->summaRadio->isChecked() && !ui->laskevalisummaanCheck->isChecked())
        kaava.append(" ==");
    if( ui->erittelyCheck->isChecked())
        kaava.append(" *");

    map.insert("L", kaava.simplified());
    return map;
}
