/*
   Copyright (C) 2017 Arto Hyvättinen

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

#include <QSqlQuery>
#include <QDebug>
#include <QPrinter>
#include <QPainter>

#include <QPushButton>
#include <QTextDocument>
#include <QMessageBox>
#include "alvilmoitusdialog.h"
#include "ui_alvilmoitusdialog.h"

#include "db/kirjanpito.h"

#include "naytin/naytinikkuna.h"

#include "alvlaskelma.h"


AlvIlmoitusDialog::AlvIlmoitusDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::AlvIlmoitusDialog)
{
    setAttribute(Qt::WA_DeleteOnClose);
    ui->setupUi(this);
    connect(ui->buttonBox, &QDialogButtonBox::helpRequested, [] { kp()->ohje("alv/tilitys");});
}

AlvIlmoitusDialog::~AlvIlmoitusDialog()
{
    delete ui;
}


void AlvIlmoitusDialog::accept()
{
    if( ui->huojennusCheck->isChecked())
        laskelma_->kirjaaHuojennus();
    connect( laskelma_, &AlvLaskelma::tallennettu, this, &AlvIlmoitusDialog::laskemaTallennettu);
    laskelma_->tallenna();
    qApp->processEvents();   
}

void AlvIlmoitusDialog::reject()
{
    laskelma_->deleteLater();
    QDialog::reject();
}


void AlvIlmoitusDialog::otsikko(const QString &teksti)
{
    RaporttiRivi rivi;
    kirjoittaja->lisaaRivi();
    rivi.lisaa(teksti);
    rivi.lihavoi();
    kirjoittaja->lisaaRivi(rivi);
}

void AlvIlmoitusDialog::luku(const QString &nimike, qlonglong senttia, bool viiva)
{
    RaporttiRivi rivi;
    rivi.lisaa(nimike);
    rivi.lisaa( senttia ,true);
    if( viiva )
        rivi.viivaYlle(true);
    kirjoittaja->lisaaRivi(rivi);
}


void AlvIlmoitusDialog::naytaLaskelma(RaportinKirjoittaja rk)
{
    laskelma_ = qobject_cast<AlvLaskelma*>( sender() );
    ui->ilmoitusBrowser->setHtml( rk.html() );    
    ui->huojennusCheck->setVisible( laskelma_->huojennus() && kp()->asetukset()->onko("AlvHuojennusTili") );
    ui->alarajaInfo->setVisible(laskelma_->huojennus() && kp()->asetukset()->onko("AlvHuojennusTili") );

    QPushButton* avaa = ui->buttonBox->addButton(tr("Tulosta"), QDialogButtonBox::ApplyRole);
    avaa->setIcon(QIcon(":/pic/print.png"));
    connect( avaa, &QPushButton::clicked, [rk] {NaytinIkkuna::naytaRaportti(rk);});

    show();

}

void AlvIlmoitusDialog::laskemaTallennettu()
{
    laskelma_->deleteLater();
    QDialog::accept();
}

