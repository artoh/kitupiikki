/*
   Copyright (C) 2026 Kitsas contributors

   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.
*/
#include "postgresvelhosivu.h"

#include <QFormLayout>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QSpinBox>
#include <QVBoxLayout>

PostgresVelhoSivu::PostgresVelhoSivu(QWidget *parent)
    : QWizardPage(parent)
{
    setTitle(tr("PostgreSQL-tietokanta"));
    setSubTitle(tr("Anna yhteystiedot tyhjään PostgreSQL-tietokantaan, johon uusi kirjanpito luodaan."));

    hostEdit_ = new QLineEdit(QStringLiteral("localhost"), this);
    portSpin_ = new QSpinBox(this);
    portSpin_->setRange(1, 65535);
    portSpin_->setValue(5432);
    databaseEdit_ = new QLineEdit(this);
    usernameEdit_ = new QLineEdit(this);
    passwordEdit_ = new QLineEdit(this);
    passwordEdit_->setEchoMode(QLineEdit::Password);

    auto *lomake = new QFormLayout;
    lomake->addRow(tr("Palvelin"), hostEdit_);
    lomake->addRow(tr("Portti"), portSpin_);
    lomake->addRow(tr("Tietokanta"), databaseEdit_);
    lomake->addRow(tr("Käyttäjätunnus"), usernameEdit_);
    lomake->addRow(tr("Salasana"), passwordEdit_);

    auto *layout = new QVBoxLayout(this);
    layout->addWidget(new QLabel(tr("Tietokannan pitää olla jo olemassa, mutta siinä ei saa olla Kitsaan kaaviota."), this));
    layout->addLayout(lomake);

    registerField("pgHost*", hostEdit_);
    registerField("pgPort", portSpin_);
    registerField("pgDatabase*", databaseEdit_);
    registerField("pgUsername", usernameEdit_);
    registerField("pgPassword", passwordEdit_);
}

PostgresYhteys PostgresVelhoSivu::yhteys() const
{
    PostgresYhteys tulos;
    tulos.host = hostEdit_->text().trimmed();
    tulos.port = portSpin_->value();
    tulos.database = databaseEdit_->text().trimmed();
    tulos.username = usernameEdit_->text().trimmed();
    tulos.password = passwordEdit_->text();
    return tulos;
}

bool PostgresVelhoSivu::validatePage()
{
    if( databaseEdit_->text().trimmed().isEmpty() ) {
        QMessageBox::warning(this, tr("PostgreSQL"), tr("Tietokannan nimi on pakollinen."));
        return false;
    }
    return true;
}
