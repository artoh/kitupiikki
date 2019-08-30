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
#include "laskulistawidget.h"
#include "ui_laskulistawidget.h"

#include "model/laskutaulumodel.h"
#include <QSortFilterProxyModel>

#include "db/kirjanpito.h"
#include "laskudialogi.h"
#include "lisaikkuna.h"

#include <QDebug>

LaskulistaWidget::LaskulistaWidget(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::LaskulistaWidget),
    laskut_(new LaskuTauluModel(this)),
    laskuAsiakasProxy_(new QSortFilterProxyModel(this)),
    laskuViiteProxy_(new QSortFilterProxyModel(this))
{
    ui->setupUi(this);

    ui->tabs->addTab(QIcon(":/pic/harmaa.png"),tr("&Lähetetyt"));
    ui->tabs->addTab(QIcon(":/pic/keltainen.png"),tr("&Avoimet"));
    ui->tabs->addTab(QIcon(":/pic/punainen.png"),tr("&Erääntyneet"));

    laskuAsiakasProxy_->setFilterCaseSensitivity(Qt::CaseInsensitive);
    laskuAsiakasProxy_->setFilterKeyColumn(LaskuTauluModel::ASIAKASTOIMITTAJA);
    laskuAsiakasProxy_->setSourceModel( laskut_ );

    laskuViiteProxy_->setSourceModel( laskuAsiakasProxy_ );
    ui->view->setModel( laskuViiteProxy_ );

    connect( ui->viiteEdit, &QLineEdit::textEdited,
             laskuViiteProxy_, &QSortFilterProxyModel::setFilterFixedString);
    connect( ui->alkupvm, &KpDateEdit::dateChanged, this, &LaskulistaWidget::paivita);
    connect( ui->loppupvm, &KpDateEdit::dateChanged, this, &LaskulistaWidget::paivita);
    connect( ui->tabs, &QTabBar::currentChanged, this, &LaskulistaWidget::paivita);

    nayta( MYYNTI );

    connect( kp(), &Kirjanpito::tietokantaVaihtui, this, &LaskulistaWidget::alusta );

    connect( ui->uusiNappi, &QPushButton::clicked, this, &LaskulistaWidget::uusilasku);
    connect( ui->muokkaaNappi, &QPushButton::clicked, this, &LaskulistaWidget::muokkaa);
}

LaskulistaWidget::~LaskulistaWidget()
{
    delete ui;
}

void LaskulistaWidget::nayta(int paalehti)
{
    paalehti_ = paalehti;

    if( paalehti == MYYNTI || paalehti == ASIAKAS) {
        if( ui->tabs->count() < 5) {
            ui->tabs->insertTab(LUONNOKSET,tr("Luonnokset"));
            ui->tabs->insertTab(LAHETETTAVAT,tr("Lähetettävät"));
            ui->tabs->setTabText(KAIKKI, tr("Lähetetyt"));
        }
    } else {
        if( ui->tabs->count() == 5) {
            ui->tabs->setTabText(KAIKKI, tr("Kaikki"));
            ui->tabs->removeTab(LAHETETTAVAT);
            ui->tabs->removeTab(LUONNOKSET);
        }
    }
}

void LaskulistaWidget::paivita()
{
    int laji = ui->tabs->count() == 5 ? ui->tabs->currentIndex() : ui->tabs->currentIndex() + 2;

    ui->view->setColumnHidden( LaskuTauluModel::NUMERO,  laji == LUONNOKSET);
    ui->view->setColumnHidden( LaskuTauluModel::MAKSAMATTA, laji < KAIKKI);
    ui->view->setColumnHidden( LaskuTauluModel::LAHETYSTAPA, laji >= KAIKKI );

    laskut_->paivita( paalehti_ == OSTO || paalehti_  == TOIMITTAJA,
                      laji, ui->alkupvm->date(), ui->loppupvm->date() );
}

void LaskulistaWidget::suodataAsiakas(const QString &nimi)
{
    laskuAsiakasProxy_->setFilterFixedString(nimi);
    qDebug() << " s " << nimi;
}

void LaskulistaWidget::alusta()
{
    ui->alkupvm->setDate(kp()->tilikaudet()->kirjanpitoAlkaa());
    ui->loppupvm->setDate(kp()->tilikaudet()->kirjanpitoLoppuu());
}

void LaskulistaWidget::uusilasku()
{
    if( paalehti_ == MYYNTI || paalehti_ == ASIAKAS) {
        LaskuDialogi *dlg = new LaskuDialogi();
        dlg->show();
    } else {
        LisaIkkuna *lisa = new LisaIkkuna(this);
        lisa->kirjaa();
    }
}

void LaskulistaWidget::muokkaa()
{
    // Hakee laskun tiedot ja näyttää dialogin
    int tositeId = ui->view->currentIndex().data(LaskuTauluModel::TositeIdRooli).toInt();
    int tyyppi = ui->view->currentIndex().data(LaskuTauluModel::TyyppiRooli).toInt();

    if( tyyppi >= TositeTyyppi::MYYNTILASKU && tyyppi <= TositeTyyppi::MAKSUMUISTUTUS) {
        KpKysely *kysely = kpk( QString("/tositteet/%1").arg(tositeId));
        connect( kysely, &KpKysely::vastaus, this, &LaskulistaWidget::naytaDialogi);
        kysely->kysy();
    } else {
        LisaIkkuna *lisa = new LisaIkkuna(this);
        lisa->naytaTosite(tositeId);
    }
}

void LaskulistaWidget::naytaDialogi(QVariant *data)
{
    LaskuDialogi* dlg = new LaskuDialogi(data->toMap());
    dlg->show();
}

