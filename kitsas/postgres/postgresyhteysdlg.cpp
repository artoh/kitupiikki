/*
   Copyright (C) 2026 Kitsas contributors

   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.
*/
#include "postgresyhteysdlg.h"
#include "ui_postgresyhteysdlg.h"

PostgresYhteysDlg::PostgresYhteysDlg(QWidget *parent)
    : QDialog(parent),
      ui(new Ui::PostgresYhteysDlg)
{
    ui->setupUi(this);
}

PostgresYhteysDlg::~PostgresYhteysDlg()
{
    delete ui;
}

void PostgresYhteysDlg::asetaYhteys(const PostgresYhteys &yhteys)
{
    ui->hostEdit->setText(yhteys.host);
    ui->portSpin->setValue(yhteys.port);
    ui->databaseEdit->setText(yhteys.database);
    ui->usernameEdit->setText(yhteys.username);
    ui->passwordEdit->setText(yhteys.password);
}

PostgresYhteys PostgresYhteysDlg::yhteys() const
{
    PostgresYhteys tulos;
    tulos.host = ui->hostEdit->text().trimmed();
    tulos.port = ui->portSpin->value();
    tulos.database = ui->databaseEdit->text().trimmed();
    tulos.username = ui->usernameEdit->text().trimmed();
    tulos.password = ui->passwordEdit->text();
    return tulos;
}
