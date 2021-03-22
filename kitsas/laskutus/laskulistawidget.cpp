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

#include "db/kirjanpito.h"
#include "lisaikkuna.h"
#include "naytin/naytinikkuna.h"
#include "maksumuistutusdialogi.h"

#include "raportti/raportinkirjoittaja.h"
#include "naytin/naytinikkuna.h"

#include "myyntilaskujentoimittaja.h"
#include "db/yhteysmodel.h"

#include "laskuproxymodel.h"
#include "laskudlg/laskudialogitehdas.h"
#include "laskudlg/kantalaskudialogi.h"
#include "model/lasku.h"

#include <QDebug>
#include <QMessageBox>

LaskulistaWidget::LaskulistaWidget(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::LaskulistaWidget),
    laskut_(new LaskuTauluModel(this)),
    proxy_(new LaskuProxyModel(this))

{
    ui->setupUi(this);

    ui->tabs->addTab(QIcon(":/pic/kaytossa.png"),tr("&Kirjanpidossa"));
    ui->tabs->addTab(QIcon(":/pic/keltainen.png"),tr("&Avoimet"));
    ui->tabs->addTab(QIcon(":/pic/punainen.png"),tr("&Erääntyneet"));
    ui->tabs->setCurrentIndex(1);   // Oletuksena avoimet

    proxy_->setSourceModel( laskut_ );
    ui->view->setModel( proxy_ );

    connect( ui->viiteEdit, &QLineEdit::textEdited,
             proxy_, &LaskuProxyModel::suodataNumerolla);

    connect( ui->alkupvm, &KpDateEdit::dateChanged, this, &LaskulistaWidget::paivita);
    connect( ui->loppupvm, &KpDateEdit::dateChanged, this, &LaskulistaWidget::paivita);
    connect( ui->tabs, &QTabBar::currentChanged, this, &LaskulistaWidget::paivita);

    nayta( MYYNTI );

    connect( kp(), &Kirjanpito::tietokantaVaihtui, this, &LaskulistaWidget::alusta );

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
    connect( ui->vainlaskuNappi, &QPushButton::toggled, proxy_, &LaskuProxyModel::suodataLaskut);

    connect( ui->tulostaButton, &QPushButton::clicked, this, &LaskulistaWidget::raportti);

    connect( laskut_, &QAbstractTableModel::modelReset, [this] { ui->view->resizeColumnToContents(LaskuTauluModel::ASIAKASTOIMITTAJA); } );

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
            ui->tabs->insertTab(LUONNOKSET,QIcon(":/pic/harmaa.png"),tr("Luonnokset"));
            ui->tabs->insertTab(LAHETETTAVAT,QIcon(":/pic/email.png"),tr("Lähetettävät"));
            ui->tabs->setTabText(KAIKKI, tr("Kirjanpidossa"));
            ui->vainlaskuNappi->setVisible(true);
            ui->lahetaNappi->setVisible(true);
            ui->kopioiNappi->setVisible(true);
        }
    } else {
        if( ui->tabs->count() == 5) {
            ui->tabs->setTabText(KAIKKI, tr("Kaikki"));
            ui->tabs->removeTab(LAHETETTAVAT);
            ui->tabs->removeTab(LUONNOKSET);
            ui->vainlaskuNappi->setVisible(false);
            ui->lahetaNappi->setVisible(false);
            ui->kopioiNappi->setVisible(false);
        }
    }
    paivita();
}

void LaskulistaWidget::alalehti(int alalehti)
{
    ui->tabs->setCurrentIndex(alalehti);
    paivita();
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

void LaskulistaWidget::suodataAsiakas(const QString &nimi, int asiakas)
{
    if(asiakas) {
        proxy_->suodataKumppani(asiakas);
    } else {
        proxy_->suodataTekstilla(nimi);
        asiakas_ = 0;
    }
}

void LaskulistaWidget::suodataViite(const QString &viite)
{
    proxy_->suodataViittella(viite);
}

void LaskulistaWidget::suodataViiteTyyppi(int viitetyyppi)
{
    proxy_->suodataViiteTyypilla(viitetyyppi);
}

void LaskulistaWidget::paivitaNapit()
{
    QModelIndex index = ui->view->selectionModel()->selectedRows().value(0);

    int tyyppi = index.data(LaskuTauluModel::TyyppiRooli).toInt();
    int laskutustapa = index.data(LaskuTauluModel::LaskutustapaRooli).toInt();

    ui->lahetaNappi->setEnabled( index.isValid()
                                 && tyyppi >= TositeTyyppi::MYYNTILASKU && tyyppi <= TositeTyyppi::MAKSUMUISTUTUS &&
                                 kp()->yhteysModel() && kp()->yhteysModel()->onkoOikeutta(YhteysModel::LASKU_LAHETTAMINEN));
    ui->kopioiNappi->setEnabled( index.isValid() && tyyppi == TositeTyyppi::MYYNTILASKU && kp()->yhteysModel() && kp()->yhteysModel()->onkoOikeutta(YhteysModel::LASKU_LAATIMINEN));

    ui->hyvitysNappi->setVisible( index.isValid()
                               && tyyppi == TositeTyyppi::MYYNTILASKU
                               && index.data(LaskuTauluModel::TunnisteRooli).toLongLong()
                               && kp()->yhteysModel() && kp()->yhteysModel()->onkoOikeutta(YhteysModel::LASKU_LAATIMINEN) );
    ui->naytaNappi->setEnabled( index.isValid() && kp()->yhteysModel() && kp()->yhteysModel()->onkoOikeutta(YhteysModel::LASKU_SELAUS));
    ui->muokkaaNappi->setEnabled( index.isValid() &&
                                  (laskutustapa != Lasku::TUOTULASKU ||
                                   tyyppi == TositeTyyppi::MYYNTILASKU));
    ui->muistutusNappi->setVisible( index.isValid() && (tyyppi == TositeTyyppi::MYYNTILASKU
                                                        || ( tyyppi == TositeTyyppi::MAKSUMUISTUTUS &&
                                                             laskutustapa != Lasku::TUOTULASKU))
                                    && index.data(LaskuTauluModel::EraPvmRooli).toDate() < kp()->paivamaara()
                                    && index.data(LaskuTauluModel::AvoinnaRooli).toDouble() > 1e-5
                                    && index.data(LaskuTauluModel::TunnisteRooli).toLongLong()
                                    && kp()->yhteysModel() && kp()->yhteysModel()->onkoOikeutta(YhteysModel::LASKU_LAATIMINEN));
    ui->ryhmalaskuNappi->setVisible(paalehti_ == MYYNTI || paalehti_ == ASIAKAS);

    ui->uusiNappi->setEnabled( kp()->yhteysModel() && kp()->yhteysModel()->onkoOikeutta(YhteysModel::LASKU_LAATIMINEN));
    ui->ryhmalaskuNappi->setEnabled( kp()->yhteysModel() && kp()->yhteysModel()->onkoOikeutta(YhteysModel::LASKU_LAATIMINEN));

    if( ui->tabs->currentIndex() >= KAIKKI )
        ui->poistaNappi->setEnabled( index.isValid() &&
                                     ( qAbs(index.data( LaskuTauluModel::SummaRooli ).toDouble() - index.data( LaskuTauluModel::AvoinnaRooli ).toDouble()) < 1e-5 || tyyppi != TositeTyyppi::MYYNTILASKU )
                                     && kp()->yhteysModel() && kp()->yhteysModel()->onkoOikeutta(YhteysModel::LASKU_LAHETTAMINEN)
                                     );
    else
        // Luonnoksen voi aina poistaa
        ui->poistaNappi->setEnabled( index.isValid() && kp()->yhteysModel() && kp()->yhteysModel()->onkoOikeutta(YhteysModel::LASKU_LAATIMINEN));
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
    if( kp()->tilitpaatetty() == kp()->tilikaudet()->kirjanpitoLoppuu() ) {
        QMessageBox::critical(this, tr("Laskua ei voi luoda"), tr("Kirjanpidossa ei ole avoinna olevaa tilikautta"));
        return;
    }

    if( paalehti_ == MYYNTI || paalehti_ == ASIAKAS) {
        KantaLaskuDialogi *dlg = LaskuDialogiTehdas::myyntilasku(asiakas_);
        connect( dlg, &KantaLaskuDialogi::tallennettuValmiina, [this] { this->ui->tabs->setCurrentIndex(LAHETETTAVAT); });
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
        LaskuDialogiTehdas::naytaLasku(tositeId);
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

void LaskulistaWidget::haettuKopioitavaksi(QVariant *data)
{
    QVariantMap map = data->toMap();
    map.remove("id");
//    map.insert("tila", Tosite::LUONNOS);
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
    umap.insert("viivkorko", lmap.value("viivkorko"));
    map.insert("lasku", umap);

//    VanhaLaskuDialogi* dlg = new VanhaLaskuDialogi(map);
//    dlg->show();
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

    QVariantMap vienti;
    vienti.insert("id", alkup.value("viennit").toList().value(0).toMap().value("id").toInt());
    QVariantList viennit;
    viennit.append(vienti);
    hyvitys.insert("viennit", viennit);

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
    hyvitys.insert("viite", alkuplasku.value("viite"));

//    VanhaLaskuDialogi* dlg = new VanhaLaskuDialogi(hyvitys);
//    dlg->show();
}

void LaskulistaWidget::raportti()
{
    QAbstractItemModel *model = ui->view->model();
    RaportinKirjoittaja rk;

    if( paalehti_ == MYYNTI || paalehti_ == ASIAKAS)
        rk.asetaOtsikko(tr("Myyntilaskut"));
    else if( paalehti_ == OSTO || paalehti_ == TOIMITTAJA)
        rk.asetaOtsikko(tr("Ostolaskut"));

    for(int i=2; i<model->columnCount(); i++)
        rk.lisaaEurosarake();
    rk.lisaaVenyvaSarake();

    RaporttiRivi otsikko;
    for(int i=0; i<model->columnCount(); i++) {
        if( i == LaskuTauluModel::LAHETYSTAPA) continue;
        otsikko.lisaa( model->headerData(i, Qt::Horizontal).toString());
    }
    rk.lisaaOtsake(otsikko);

    for(int i=0; i<model->rowCount();i++) {
        RaporttiRivi rivi;
        for(int j=0; j<model->columnCount(); j++) {
            if( j == LaskuTauluModel::LAHETYSTAPA) continue;
            QString teksti = model->index(i,j).data().toString();
            if(QDate::fromString(teksti,Qt::ISODate).isValid())
                rivi.lisaa(QDate::fromString(teksti, Qt::ISODate));
            else
                rivi.lisaa(teksti,1, teksti.contains("€"));
        }
        rk.lisaaRivi(rivi);
    }

    NaytinIkkuna::naytaRaportti(rk);
}

