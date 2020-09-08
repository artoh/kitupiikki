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
#include "emailkokeilu.h"
#include "ui_emailkokeilu.h"

#include "laskutus/smtp.h"
#include <QTimer>

EmailKokeilu::EmailKokeilu(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::EmailKokeilu)
{
    ui->setupUi(this);
    setAttribute(Qt::WA_DeleteOnClose);

    ui->progressBar->setRange(0,60);
    ui->progressBar->setValue(0);
    QTimer* sekunti = new QTimer(this);
    sekunti->start(1000);
    connect( sekunti, &QTimer::timeout, this, &EmailKokeilu::sekunti);
}

EmailKokeilu::~EmailKokeilu()
{
    delete ui;
}

void EmailKokeilu::status(int status)
{
    switch (status) {
    case Smtp::Connecting:
        ui->statusLabel->setText( tr("Yhdistetään sähköpostipalvelimeen..."));
        break;
    case Smtp::Sending:
        ui->statusLabel->setText(tr("Lähetetään sähköpostia..."));
        break;
    case Smtp::Send:
        ui->statusLabel->setText(tr("Sähköposti lähetetty."));
        ui->statusLabel->setStyleSheet("color: green;");
        ui->progressBar->hide();
        break;
    case Smtp::Failed:
        ui->statusLabel->setText(tr("Sähköpostin lähettäminen epäonnistui."));
        ui->statusLabel->setStyleSheet("color: red;");
        ui->progressBar->hide();
        break;
    }
}

void EmailKokeilu::debug(const QString &data)
{
    ui->textBrowser->insertPlainText(data + "\n");
}

void EmailKokeilu::sekunti()
{
    if( ui->progressBar->value() < 30)
        ui->progressBar->setValue( ui->progressBar->value() + 1 );
}
