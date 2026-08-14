/*
   Copyright (C) 2026 Kitsas contributors

   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.
*/
#include "selectclientdlg.h"
#include "ui_selectclientdlg.h"

#include "db/kirjanpito.h"
#include "postgresmodel.h"
#include "uusikirjanpito/uusivelho.h"

#include <QInputDialog>
#include <QMessageBox>
#include <QPushButton>

SelectClientDlg::SelectClientDlg(const PostgresYhteys &palvelin, QWidget *parent)
    : QDialog(parent),
      ui(new Ui::SelectClientDlg),
      palvelin_(palvelin)
{
    ui->setupUi(this);

    connect(ui->clientList, &QListWidget::itemSelectionChanged, this, &SelectClientDlg::valintaMuuttui);
    connect(ui->clientList, &QListWidget::itemDoubleClicked, this, &SelectClientDlg::avaaValittu);
    connect(ui->newClientButton, &QPushButton::clicked, this, &SelectClientDlg::uusiAsiakas);
    connect(ui->buttonBox, &QDialogButtonBox::accepted, this, &SelectClientDlg::avaaValittu);

    paivitaLista();
}

SelectClientDlg::~SelectClientDlg()
{
    delete ui;
}

void SelectClientDlg::paivitaLista()
{
    const QString valittu = ui->clientList->currentItem()
            ? ui->clientList->currentItem()->text()
            : QString();

    ui->clientList->clear();
    const QStringList nimet = kp()->postgres()->listaaTietokannat(palvelin_);
    ui->clientList->addItems(nimet);

    if( !valittu.isEmpty()) {
        const QList<QListWidgetItem*> osumat = ui->clientList->findItems(valittu, Qt::MatchExactly);
        if( !osumat.isEmpty())
            ui->clientList->setCurrentItem(osumat.first());
    } else if( ui->clientList->count() > 0) {
        ui->clientList->setCurrentRow(0);
    }

    valintaMuuttui();
}

void SelectClientDlg::valintaMuuttui()
{
    if( QPushButton *ok = ui->buttonBox->button(QDialogButtonBox::Ok) )
        ok->setEnabled(ui->clientList->currentItem() != nullptr);
}

PostgresYhteys SelectClientDlg::valittuYhteys() const
{
    if( !ui->clientList->currentItem())
        return PostgresYhteys();
    return palvelin_.asiakasYhteys(ui->clientList->currentItem()->text());
}

void SelectClientDlg::avaaValittu()
{
    const PostgresYhteys yhteys = valittuYhteys();
    if( yhteys.database.isEmpty())
        return;

    if( kp()->postgres()->avaa(yhteys) ) {
        joAvattu_ = true;
        accept();
    }
}

void SelectClientDlg::uusiAsiakas()
{
    bool ok = false;
    const QString nimi = QInputDialog::getText(this, tr("New Client"),
                                               tr("Client name (database name):"),
                                               QLineEdit::Normal, QString(), &ok)
            .trimmed().toLower();
    if( !ok || nimi.isEmpty())
        return;

    if( !PostgresModel::onkoKelvollinenTietokannanNimi(nimi) ) {
        QMessageBox::warning(this, tr("New Client"),
                             tr("Client name must start with a letter and contain only letters, numbers and underscores (max 63 characters)."));
        return;
    }

    if( ui->clientList->findItems(nimi, Qt::MatchExactly).count() ) {
        QMessageBox::warning(this, tr("New Client"),
                             tr("A client named %1 already exists.").arg(nimi));
        return;
    }

    UusiVelho velho(this);
    if( !velho.uusiPostgresAsiakas(palvelin_.asiakasYhteys(nimi)) )
        return;

    if( !kp()->postgres()->luoTietokanta(palvelin_, nimi) )
        return;

    if( kp()->postgres()->uusiKirjanpito(palvelin_.asiakasYhteys(nimi), velho.data()) ) {
        joAvattu_ = true;
        accept();
    }
}
