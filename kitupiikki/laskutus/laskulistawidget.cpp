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
#include "naytin/naytinikkuna.h"
#include "maksumuistutusdialogi.h"
#include <QDebug>

#include "myyntilaskujentoimittaja.h"

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
    connect( kp(), &Kirjanpito::kirjanpitoaMuokattu, this, &LaskulistaWidget::paivita );

    connect( ui->naytaNappi, &QPushButton::clicked, this, &LaskulistaWidget::naytaLasku);
    connect( ui->kopioiNappi, &QPushButton::clicked, this, &LaskulistaWidget::kopioi);
    connect( ui->lahetaNappi, &QPushButton::clicked, this, &LaskulistaWidget::laheta);

    connect( ui->uusiNappi, &QPushButton::clicked, [this] {this->uusilasku(false);});
    connect( ui->ryhmalaskuNappi, &QPushButton::clicked, [this] {this->uusilasku(true);});

    connect( ui->muokkaaNappi, &QPushButton::clicked, this, &LaskulistaWidget::muokkaa);    
    connect( ui->poistaNappi, &QPushButton::clicked, this, &LaskulistaWidget::poista);

    connect( ui->hyvitysNappi, &QPushButton::clicked, this, &LaskulistaWidget::hyvita);
    connect( ui->muistutusNappi, &QPushButton::clicked, this, &LaskulistaWidget::muistuta);

    connect( ui->view, &QTableView::doubleClicked, this, &LaskulistaWidget::muokkaa);

    connect( ui->view->selectionModel(), &QItemSelectionModel::selectionChanged, this, &LaskulistaWidget::paivitaNapit);
    connect( ui->view->selectionModel(), &QItemSelectionModel::currentRowChanged, this, &LaskulistaWidget::paivitaNapit);

    // Nämä toistaiseksi poissa käytöstä
    ui->muistutusNappi->hide();
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

//    ui->view->setColumnHidden( LaskuTauluModel::NUMERO,  laji < KAIKKI);
    ui->view->setColumnHidden( LaskuTauluModel::PVM, laji < KAIKKI);
    ui->view->setColumnHidden( LaskuTauluModel::MAKSAMATTA, laji < KAIKKI);
    ui->view->setColumnHidden( LaskuTauluModel::LAHETYSTAPA, laji >= KAIKKI );

    laskut_->paivita( paalehti_ == OSTO || paalehti_  == TOIMITTAJA,
                      laji, ui->alkupvm->date(), ui->loppupvm->date() );

    paivitaNapit();
}

void LaskulistaWidget::suodataAsiakas(const QString &nimi)
{
    laskuAsiakasProxy_->setFilterFixedString(nimi);
}

void LaskulistaWidget::paivitaNapit()
{
    QModelIndex index = ui->view->selectionModel()->selectedRows().value(0);

    ui->lahetaNappi->setEnabled( index.isValid() );
    ui->kopioiNappi->setEnabled( index.isValid() );
    ui->hyvitysNappi->setVisible( index.isValid()
                               && index.data(LaskuTauluModel::TyyppiRooli).toInt() == TositeTyyppi::MYYNTILASKU
                               && index.data(LaskuTauluModel::TunnisteRooli).toLongLong());
    ui->naytaNappi->setEnabled( index.isValid() );
    ui->muokkaaNappi->setEnabled( index.isValid() );
    ui->muistutusNappi->setVisible( index.isValid() && (index.data(LaskuTauluModel::TyyppiRooli).toInt() == TositeTyyppi::MYYNTILASKU
                                                        || index.data(LaskuTauluModel::TyyppiRooli).toInt() == TositeTyyppi::MAKSUMUISTUTUS) && index.data(LaskuTauluModel::EraPvmRooli).toDate() < kp()->paivamaara() );
    ui->ryhmalaskuNappi->setVisible(paalehti_ == MYYNTI || paalehti_ == ASIAKAS);

    if( ui->tabs->currentIndex() >= KAIKKI )
        ui->poistaNappi->setEnabled( index.isValid() &&
                                     index.data( LaskuTauluModel::AvoinnaRooli ).toDouble() > 1e-5);
    else
        // Luonnoksen voi aina poistaa
        ui->poistaNappi->setEnabled( index.isValid() );
}

void LaskulistaWidget::laheta()
{
    MyyntiLaskujenToimittaja *toimittaja = new MyyntiLaskujenToimittaja(this);
    connect( toimittaja, &MyyntiLaskujenToimittaja::laskutToimitettu, this, &LaskulistaWidget::paivita );

    QModelIndexList lista = ui->view->selectionModel()->selectedRows();
    QList<int> idt;
    for( auto item : lista)
        idt.append( item.data(LaskuTauluModel::TositeIdRooli).toInt() );

    toimittaja->toimitaLaskut( idt);

}

void LaskulistaWidget::alusta()
{
    ui->alkupvm->setDate(kp()->tilikaudet()->kirjanpitoAlkaa());
    ui->loppupvm->setDate(kp()->tilikaudet()->kirjanpitoLoppuu());
}

void LaskulistaWidget::uusilasku(bool ryhmalasku)
{
    if( kp()->paivamaara() <= kp()->tilitpaatetty() ||
        kp()->paivamaara() > kp()->tilikaudet()->kirjanpitoLoppuu()) {
        QMessageBox::critical(this, tr("Laskua ei voi luoda"), tr("Et voi luoda uutta laskua, koska nykyiselle päivälle ei ole avoinna olevaa tilikautta"));
        return;
    }

    if( paalehti_ == MYYNTI || paalehti_ == ASIAKAS) {
        LaskuDialogi *dlg = new LaskuDialogi(QVariantMap(), ryhmalasku);
        dlg->show();
    } else {
        LisaIkkuna *lisa = new LisaIkkuna(this);
        lisa->kirjaa(-1, TositeTyyppi::TULO);
    }
}

void LaskulistaWidget::muokkaa()
{
    // Hakee laskun tiedot ja näyttää dialogin
    int tositeId = ui->view->selectionModel()->selectedRows().value(0).data(LaskuTauluModel::TositeIdRooli).toInt();
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

void LaskulistaWidget::kopioi()
{
    int tositeId = ui->view->selectionModel()->selectedRows().value(0).data(LaskuTauluModel::TositeIdRooli).toInt();
    if( tositeId ) {
        KpKysely* kysely = kpk( QString("/tositteet/%1").arg(tositeId));
        connect(kysely, &KpKysely::vastaus, this, &LaskulistaWidget::haettuKopioitavaksi);
        kysely->kysy();
    }
}

void LaskulistaWidget::hyvita()
{
    int tositeId = ui->view->selectionModel()->selectedRows().value(0).data(LaskuTauluModel::TositeIdRooli).toInt();
    if( tositeId ) {
        KpKysely* kysely = kpk( QString("/tositteet/%1").arg(tositeId));
        connect(kysely, &KpKysely::vastaus, this, &LaskulistaWidget::teeHyvitysLasku);
        kysely->kysy();
    }
}

void LaskulistaWidget::muistuta()
{
    QList<int> erat;
    for(auto item : ui->view->selectionModel()->selectedRows()) {
        int eraId = item.data(LaskuTauluModel::EraIdRooli).toInt();
        erat.append(eraId);
    }
    new MaksumuistutusDialogi(erat, this);
}

void LaskulistaWidget::poista()
{
    QModelIndex index = ui->view->selectionModel()->selectedRows().value(0);
    int tositeId = index.data(LaskuTauluModel::TositeIdRooli).toInt();
    if( tositeId ) {
        QString viitenumero = index.data(LaskuTauluModel::ViiteRooli).toString();
        QString asiakas = index.data(LaskuTauluModel::AsiakasToimittajaNimiRooli).toString();
        if( QMessageBox::question(this, tr("Laskun poistaminen"),
                                  tr("Haluatko todella poistaa laskun%1%2?")
                                  .arg( viitenumero.isEmpty() ? "" : tr(" viitenumerolla %1").arg(viitenumero)  )
                                  .arg( asiakas.isEmpty() ? "" : tr(" asiakkaalle %1").arg(asiakas) ) )
                == QMessageBox::Yes) {
            KpKysely *kysely = kpk(QString("/tositteet/%1").arg(tositeId), KpKysely::DELETE );
            connect( kysely, &KpKysely::vastaus, this, &LaskulistaWidget::paivita );
            kysely->kysy();
        }
    }

}

void LaskulistaWidget::naytaLasku()
{
    int tositeId = ui->view->selectionModel()->selectedRows().value(0).data(LaskuTauluModel::TositeIdRooli).toInt();
    int tyyppi = ui->view->currentIndex().data(LaskuTauluModel::TyyppiRooli).toInt();
    if( tyyppi >= TositeTyyppi::MYYNTILASKU && tyyppi <= TositeTyyppi::MAKSUMUISTUTUS && tositeId) {
        NaytinIkkuna::naytaLiite(tositeId,"lasku");
    } else if( tositeId) {
        LisaIkkuna *lisa = new LisaIkkuna(this);
        lisa->naytaTosite(tositeId);
    }
}

void LaskulistaWidget::naytaDialogi(QVariant *data)
{
    LaskuDialogi* dlg = new LaskuDialogi(data->toMap());
    dlg->show();
}

void LaskulistaWidget::haettuKopioitavaksi(QVariant *data)
{
    QVariantMap map = data->toMap();
    map.remove("id");
    map.insert("tila", Tosite::LUONNOS);
    map.remove("viennit");
    map.remove("loki");
    QVariantMap lmap = map.take("lasku").toMap();

    QVariantMap umap;
    umap.insert("kieli", lmap.value("kieli"));
    umap.insert("laskutapa", lmap.value("laskutapa"));
    umap.insert("maksutapa", lmap.value("maksutapa"));
    umap.insert("otsikko", lmap.value("otsikko"));
    umap.insert("osoite", lmap.value("osoite"));
    umap.insert("email", lmap.value("email"));
    umap.insert("toimituspvm", kp()->paivamaara());
    umap.insert("erapvm", kp()->paivamaara().addDays( kp()->asetukset()->luku("LaskuMaksuaika") ));
    map.insert("lasku", umap);

    LaskuDialogi* dlg = new LaskuDialogi(map);
    dlg->show();
}

void LaskulistaWidget::teeHyvitysLasku(QVariant *data)
{
    QVariantMap alkup = data->toMap();
    QVariantMap hyvitys;


    QVariantMap alkuplasku = alkup.value("lasku").toMap();
    QVariantMap lasku;
    lasku.insert("kieli", alkuplasku.value("kieli"));
    lasku.insert("laskutapa", alkuplasku.value("laskutapa"));
    lasku.insert("alkupNro", alkuplasku.value("numero"));
    lasku.insert("osoite", alkuplasku.value("osoite"));
    lasku.insert("email", alkuplasku.value("email"));
    lasku.insert("alkupPvm", alkup.value("pvm"));
    lasku.insert("viite", alkuplasku.value("viite"));
    if(alkuplasku.contains("alvtunnus"))
        lasku.insert("alvtunnus", alkuplasku.value("alvtunnus"));
    lasku.insert("era", alkup.value("viennit").toList().value(0).toMap().value("id").toInt());
    hyvitys.insert("lasku", lasku);

    hyvitys.insert("kumppani", alkup.value("kumppani"));
    hyvitys.insert("tyyppi", TositeTyyppi::HYVITYSLASKU);

    QVariantList alkuprivit = alkup.value("rivit").toList();
    QVariantList rivit;
    for( auto item : alkuprivit) {
        QVariantMap rmap = item.toMap();
        rmap.insert("myyntikpl", 0-rmap.value("myyntikpl").toDouble());
        rivit.append(rmap);
    }
    hyvitys.insert("rivit", rivit);

    LaskuDialogi* dlg = new LaskuDialogi(hyvitys);
    dlg->show();
}

