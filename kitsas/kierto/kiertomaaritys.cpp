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
#include "kiertomaaritys.h"
#include "ui_kiertomaaritys.h"
#include "db/kirjanpito.h"
#include "kiertomodel.h"
#include "db/kirjanpito.h"
#include "pilvi/pilvimodel.h"
#include "kiertomuokkausdlg.h"

#include <QSortFilterProxyModel>
#include <QMessageBox>

KiertoMaaritys::KiertoMaaritys(QWidget *parent) :
    MaaritysWidget(parent),
    ui(new Ui::KiertoMaaritys)
{
    ui->setupUi(this);

    QSortFilterProxyModel *proxy = new QSortFilterProxyModel(this);
    proxy->setSourceModel(kp()->kierrot());
    proxy->setSortRole(KiertoModel::NimiRooli);
    ui->view->setModel(proxy);

    QString osoite = kp()->pilvi()->pilviLoginOsoite();
    osoite = osoite.left(osoite.lastIndexOf('/'));


    ui->osoiteEdit->setText( QString("%1/portaali/%2")
                              .arg(osoite)
                              .arg(kp()->pilvi()->pilviId()));

    connect(ui->uusiNappi, &QPushButton::clicked, this, &KiertoMaaritys::uusi);
    connect(ui->muokkaaNappi, &QPushButton::clicked, this, &KiertoMaaritys::muokkaa);
    connect(ui->poistaNappi, &QPushButton::clicked, this, &KiertoMaaritys::poista);

    connect(ui->alatunnisteEdit, &QPlainTextEdit::textChanged, this, &KiertoMaaritys::ohjeMuokattu);
    connect(ui->ocrCheck, &QCheckBox::clicked, this, &KiertoMaaritys::ohjeMuokattu);
    connect(ui->tallennaNappi, &QPushButton::clicked, [this] () {
        kp()->asetukset()->aseta("PortaaliOhje", ui->alatunnisteEdit->toPlainText());
        kp()->asetukset()->aseta("PortaaliOCR", ui->ocrCheck->isChecked());
        this->ohjeMuokattu();
    });
    connect(ui->PeruNappi, &QPushButton::clicked, this, [this] () {
        this->ui->alatunnisteEdit->setPlainText(kp()->asetukset()->asetus("PortaaliOhje"));
    });
    connect(ui->portaaliRyhma, &QGroupBox::toggled, [] (bool paalla) {
        kp()->asetukset()->aseta("Portaali", paalla);
    });

    connect(ui->view->selectionModel(), &QItemSelectionModel::currentChanged, this, &KiertoMaaritys::paivitaNapit);
}

KiertoMaaritys::~KiertoMaaritys()
{
    delete ui;
}

void KiertoMaaritys::uusi()
{
    KiertoMuokkausDlg dlg(0, this, ui->portaaliRyhma->isChecked());
    dlg.exec();
    kp()->kierrot()->paivita();
}

void KiertoMaaritys::muokkaa()
{
    int id = ui->view->currentIndex().data(KiertoModel::IdRooli).toInt();
    if( id ) {
        KiertoMuokkausDlg dlg(id, this, ui->portaaliRyhma->isChecked());
        dlg.exec();
        kp()->kierrot()->paivita();

    }
}

void KiertoMaaritys::poista()
{
    int id = ui->view->currentIndex().data(KiertoModel::IdRooli).toInt();
    if( id == 1){
        QMessageBox::information(this, tr("Verkkomaksujen kierron poistaminen"),
                              tr("Tämä kierto on verkkolaskujen vastaanottamista varten, eikä sitä voi poistaa."));
        return;
    }
    if( id &&
            QMessageBox::question(this, tr("Kierron poistaminen"),
                                  tr("Haluatko todella poistaa kierron %1")
                                  .arg(ui->view->currentIndex().data(KiertoModel::NimiRooli).toString())
                                  ) == QMessageBox::Yes) {

        KpKysely *poisto = kpk(QString("/kierrot/%1").arg(id), KpKysely::DELETE);
        connect(poisto, &KpKysely::vastaus, kp()->kierrot(), &KiertoModel::paivita);
        poisto->kysy();
    }
}

bool KiertoMaaritys::nollaa()
{
    ui->portaaliRyhma->setChecked( kp()->asetukset()->onko("Portaali") );
    ui->alatunnisteEdit->setPlainText( kp()->asetukset()->asetus("PortaaliOhje"));
    ui->ocrCheck->setChecked( kp()->asetukset()->onko("PortaaliOCR") );
    return true;
}

void KiertoMaaritys::ohjeMuokattu()
{
    bool muokattu = kp()->asetukset()->asetus("PortaaliOhje") != ui->alatunnisteEdit->toPlainText() ||
            ui->ocrCheck->isChecked() != kp()->asetukset()->onko("PortaaliOCR");
    ui->tallennaNappi->setEnabled( muokattu);
    ui->PeruNappi->setEnabled( muokattu );
}

void KiertoMaaritys::paivitaNapit(const QModelIndex& index)
{
    ui->muokkaaNappi->setEnabled( index.isValid());
    ui->poistaNappi->setEnabled( index.isValid());
}
