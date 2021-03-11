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
#include "kantalaskudialogi.h"
#include "ui_laskudialogi.h"

#include "model/tosite.h"

#include "db/kirjanpito.h"

#include "db/tositetyyppimodel.h"

#include "../ennakkohyvitysmodel.h"
#include "../ryhmalasku/kielidelegaatti.h"
#include "model/tositerivit.h"
#include "model/tositeloki.h"
#include "../tuotedialogi.h"

#include "../laskutusverodelegaatti.h"
#include "kirjaus/eurodelegaatti.h"
#include "kirjaus/kohdennusdelegaatti.h"
#include "kirjaus/tilidelegaatti.h"

#include "naytin/naytinikkuna.h"
#include "../viitenumero.h"

#include "../vakioviite/vakioviitemodel.h"
#include "../huoneisto/huoneistomodel.h"

#include <QJsonDocument>
#include <QMenu>
#include <QSettings>
#include <QSortFilterProxyModel>



KantaLaskuDialogi::KantaLaskuDialogi(Tosite *tosite, QWidget *parent) :
    QDialog(parent),
    ui( new Ui::LaskuDialogi),
    tosite_(tosite),
    ennakkoModel_(new EnnakkoHyvitysModel(this)),
    huoneistot_(new HuoneistoModel(this))
{
    ui->setupUi(this);
    tosite_->setParent(this);

    setAttribute(Qt::WA_DeleteOnClose);
    resize(800,600);
    restoreGeometry( kp()->settings()->value("LaskuDialogi").toByteArray());

    alustaUi();
    teeConnectit();

}

KantaLaskuDialogi::~KantaLaskuDialogi() {
    kp()->settings()->setValue("LaskuDialogi", saveGeometry());
    delete ui;
}


void KantaLaskuDialogi::alustaUi()
{
    KieliDelegaatti::alustaKieliCombo(ui->kieliCombo);
    ui->jaksoDate->setNull();

    ui->viiteLabel->hide();
    ui->viiteText->hide();
    ui->maksettuCheck->hide();
    ui->infoLabel->hide();

    ui->maksuaikaSpin->setValue(kp()->asetukset()->luku("LaskuMaksuaika",14) );
    ui->saateEdit->setPlainText( kp()->asetus("EmailSaate") );
    ui->viivkorkoSpin->setValue( kp()->asetus("LaskuPeruskorko").toDouble() + 7.0 );

    int lokiIndex = ui->tabWidget->indexOf( ui->tabWidget->findChild<QWidget*>("loki") );
    ui->tabWidget->setTabEnabled( lokiIndex, false);

    ui->lokiView->setModel( tosite_->loki() );

    paivitaLaskutustavat();
    alustaMaksutavat();
    alustaValvonta();
}

void KantaLaskuDialogi::alustaRivitTab()
{
    QSortFilterProxyModel *proxy = new QSortFilterProxyModel(this);
    proxy->setSourceModel( kp()->tuotteet() );

    ui->tuoteView->setModel(proxy);
    proxy->setSortLocaleAware(true);
    proxy->setFilterCaseSensitivity(Qt::CaseInsensitive);
    ui->tuoteView->sortByColumn(TuoteModel::NIMIKE, Qt::AscendingOrder);
    ui->tuoteView->horizontalHeader()->setSectionResizeMode(TuoteModel::NIMIKE, QHeaderView::Stretch);
    ui->tuoteView->setContextMenuPolicy(Qt::CustomContextMenu);
    connect( ui->tuoteView, &QTableView::customContextMenuRequested,
             this, &KantaLaskuDialogi::tuotteidenKonteksiValikko);


    ui->rivitView->setModel(tosite()->rivit());

    ui->rivitView->horizontalHeader()->setSectionResizeMode(TositeRivit::NIMIKE, QHeaderView::Stretch);
    ui->rivitView->setItemDelegateForColumn(TositeRivit::AHINTA, new EuroDelegaatti());
    ui->rivitView->setItemDelegateForColumn(TositeRivit::TILI, new TiliDelegaatti());

    KohdennusDelegaatti *kohdennusDelegaatti = new KohdennusDelegaatti(this);
    kohdennusDelegaatti->asetaKohdennusPaiva(ui->toimitusDate->date());
    ui->rivitView->setItemDelegateForColumn(TositeRivit::KOHDENNUS, kohdennusDelegaatti );

    connect( ui->toimitusDate , SIGNAL(dateChanged(QDate)), kohdennusDelegaatti, SLOT(asetaKohdennusPaiva(QDate)));
    connect( ui->tuoteFiltterinEditori, &QLineEdit::textChanged, proxy, &QSortFilterProxyModel::setFilterFixedString);

    ui->rivitView->setItemDelegateForColumn(TositeRivit::BRUTTOSUMMA, new EuroDelegaatti());
//    ui->rivitView->setItemDelegateForColumn(TositeRivit::ALV, new LaskutusVeroDelegaatti(this));

    ui->rivitView->setColumnHidden( TositeRivit::ALV, !kp()->asetukset()->onko("AlvVelvollinen") );
    ui->rivitView->setColumnHidden( TositeRivit::KOHDENNUS, !kp()->kohdennukset()->kohdennuksia());

    connect( ui->uusituoteNappi, &QPushButton::clicked, [this] { (new TuoteDialogi(this))->uusi(); } );
    connect( ui->lisaaRiviNappi, &QPushButton::clicked, [this] { this->tosite()->rivit()->lisaaRivi();} );
    connect( ui->poistaRiviNappi, &QPushButton::clicked, [this] {
        if( this->ui->rivitView->currentIndex().isValid())
                this->tosite()->rivit()->poistaRivi( ui->rivitView->currentIndex().row());
    });

    ui->splitter->setStretchFactor(0,1);
    ui->splitter->setStretchFactor(1,3);

    connect( ui->tuoteView, &QTableView::clicked, [this] (const QModelIndex& index)
        { this->tosite()->rivit()->lisaaRivi( index.data(TuoteModel::TuoteMapRooli).toMap() ); }  );
}


void KantaLaskuDialogi::teeConnectit()
{
    connect( ui->asiakas, &AsiakasToimittajaValinta::muuttui, this, &KantaLaskuDialogi::asiakasMuuttui);
    connect( ui->asiakas, &AsiakasToimittajaValinta::valittu, this, &KantaLaskuDialogi::asiakasMuuttui);

    connect( ui->email, &QLineEdit::textChanged, this, &KantaLaskuDialogi::paivitaLaskutustavat);
    connect( ui->osoiteEdit, &QPlainTextEdit::textChanged, this, &KantaLaskuDialogi::paivitaLaskutustavat);
    connect( ui->laskutusCombo, &QComboBox::currentTextChanged, this, &KantaLaskuDialogi::laskutusTapaMuuttui);
    connect( ui->maksuCombo, &QComboBox::currentTextChanged, this, &KantaLaskuDialogi::maksuTapaMuuttui);
    connect( ui->valvontaCombo, &QComboBox::currentTextChanged, this, &KantaLaskuDialogi::valvontaMuuttui);
    connect( ui->tarkeCombo, &QComboBox::currentTextChanged, this, &KantaLaskuDialogi::paivitaViiteRivi);
}

void KantaLaskuDialogi::alustaMaksutavat()
{
    ui->maksuCombo->clear();
    ui->maksuCombo->addItem(QIcon(":/pic/lasku.png"), tr("Lasku"), Lasku::LASKU);
    if( tosite()->tyyppi() == TositeTyyppi::MYYNTILASKU) {
        ui->maksuCombo->addItem(QIcon(":/pic/kateinen.png"), tr("Käteinen"), Lasku::KATEINEN);
        ui->maksuCombo->addItem(QIcon(":/pic/ennakkolasku.png"), tr("Ennakkolasku"), Lasku::ENNAKKOLASKU);
        ui->maksuCombo->addItem(QIcon(":/pic/suorite.png"), tr("Suoriteperusteinen lasku"), Lasku::SUORITEPERUSTE);

        ui->maksuCombo->addItem(QIcon(":/pic/refresh.png"), tr("Kuukausittainen lasku"), Lasku::KUUKAUSITTAINEN);
    }
}

void KantaLaskuDialogi::alustaValvonta()
{
    ui->valvontaCombo->addItem(QIcon(":/pic/lasku.png"), tr("Yksittäinen lasku"), Lasku::LASKUVALVONTA);
    ui->valvontaCombo->addItem(QIcon(":/pic/mies.png"), tr("Asiakas"), Lasku::ASIAKAS);
    ui->valvontaCombo->addItem(QIcon(":/pic/talo.png"), tr("Huoneisto"), Lasku::HUONEISTO);
    ui->valvontaCombo->addItem(QIcon(":/pic/viivakoodi.png"), tr("Vakioviite"), Lasku::VAKIOVIITE);
    ui->valvontaCombo->addItem(QIcon(":/pic/eikaytossa.png"), tr("Valvomaton"), Lasku::VALVOMATON);
}

void KantaLaskuDialogi::tuotteidenKonteksiValikko(QPoint pos)
{
    QModelIndex index = ui->tuoteView->indexAt(pos);
    int tuoteid = index.data(TuoteModel::IdRooli).toInt();
    QVariantMap tuoteMap = index.data(TuoteModel::MapRooli).toMap();

    QMenu *menu = new QMenu(this);
    menu->addAction(QIcon(":/pic/muokkaa.png"), tr("Muokkaa"), [this, tuoteMap] {
        TuoteDialogi* dlg = new TuoteDialogi(this);
        dlg->muokkaa( tuoteMap );
    });
    menu->addAction(QIcon(":/pic/refresh.png"), tr("Päivitä luettelo"), [] {
        kp()->tuotteet()->lataa();
    });
    if( tuoteid )
        menu->popup( ui->tuoteView->viewport()->mapToGlobal(pos));
}

void KantaLaskuDialogi::tositteelta()
{
    const Lasku& lasku = tosite()->constLasku();

    ui->asiakas->set( tosite()->kumppani(), tosite()->kumppaninimi() );

    ui->osoiteEdit->setPlainText( lasku.osoite());
    ui->email->setText( lasku.email());


    int lokiIndex = ui->tabWidget->indexOf( ui->tabWidget->findChild<QWidget*>("loki") );
    ui->tabWidget->setTabEnabled( lokiIndex, tosite()->loki()->rowCount() );

}

void KantaLaskuDialogi::asiakasMuuttui()
{
    const int asiakasId = ui->asiakas->nimi().isEmpty() ? 0 :  ui->asiakas->id();
    ui->osoiteEdit->setEnabled( asiakasId == 0);
    if( asiakasId && asiakasId != asiakasId_) {
        KpKysely *kysely = kpk( QString("/kumppanit/%1").arg(asiakasId) );
        connect( kysely, &KpKysely::vastaus, this, &KantaLaskuDialogi::taytaAsiakasTiedot);
        kysely->kysy();
    } else if( !asiakasId && asiakasId_) {
        ladattuAsiakas_.clear();
        ui->osoiteEdit->clear();
        ui->email->clear();
        paivitaLaskutustavat();
    }
    asiakasId_ = asiakasId;
}

void KantaLaskuDialogi::taytaAsiakasTiedot(QVariant *data)
{
    QVariantMap map = data->toMap();
    ladattuAsiakas_ = map;

    ui->osoiteEdit->setPlainText( map.value("nimi").toString() + "\n" +
                                  map.value("osoite").toString() + "\n" +
                                  map.value("postinumero").toString() + " " + map.value("kaupunki").toString());

    ui->email->setText( map.value("email").toString());
    ui->kieliCombo->setCurrentIndex(ui->kieliCombo->findData(map.value("kieli","FI").toString()));

    paivitaLaskutustavat();
    const int laskutapaIndeksi = map.contains("laskutapa") ? ui->laskutusCombo->findData(map.value("laskutapa", -1)) : -1;
    if( laskutapaIndeksi > -1)
        ui->laskutusCombo->setCurrentIndex(laskutapaIndeksi);

    if( map.contains("maksuaika")) {
        ui->maksuaikaSpin->setValue(map.value("maksuaika").toInt());
    }

    if( map.value("alvtunnus").toString().isEmpty())
        // Yksityishenkilön viivästyskorko on peruskorko + 7 %
        ui->viivkorkoSpin->setValue( kp()->asetus("LaskuPeruskorko").toDouble() + 7.0 );
    else
        // Yrityksen viivästyskorko on peruskorko + 8 %
        ui->viivkorkoSpin->setValue( kp()->asetus("LaskuPeruskorko").toDouble() + 8.0 );
}

void KantaLaskuDialogi::paivitaLaskutustavat()
{
    if(paivitetaanLaskutapoja_) {
        return;
    }
    paivitetaanLaskutapoja_ = true;
    int nykyinen = ui->laskutusCombo->currentData().toInt();
    ui->laskutusCombo->clear();

    ui->laskutusCombo->addItem( QIcon(":/pic/tulosta.png"), tr("Tulosta lasku"), Lasku::TULOSTETTAVA);

    // Voidaan postittaa vain jos osoite asiakasrekisterissä
    if( ladattuAsiakas_.value("nimi").toString().length() > 1 &&
        ladattuAsiakas_.value("osoite").toString().length() > 1 &&
        ladattuAsiakas_.value("postinumero").toString().length() > 1 &&
        ladattuAsiakas_.value("kaupunki").toString() > 1 )
        ui->laskutusCombo->addItem( QIcon(":/pic/mail.png"), tr("Postita lasku"), Lasku::POSTITUS);

    if( kp()->asetukset()->luku("FinvoiceKaytossa") &&
        ladattuAsiakas_.value("ovt").toString().length() > 9 &&
        ladattuAsiakas_.value("operaattori").toString().length() > 4 )
        ui->laskutusCombo->addItem( QIcon(":/pic/verkkolasku.png"), tr("Verkkolasku"), Lasku::VERKKOLASKU);

    QRegularExpression emailRe(R"(^.*@.*\.\w+$)");
    if( emailRe.match( ui->email->text()).hasMatch() )
            ui->laskutusCombo->addItem(QIcon(":/pic/email.png"), tr("Lähetä sähköpostilla"), Lasku::SAHKOPOSTI);
    ui->laskutusCombo->addItem( QIcon(":/pic/pdf.png"), tr("Tallenna pdf-tiedostoon"), Lasku::PDF);
    ui->laskutusCombo->addItem( QIcon(":/pic/tyhja.png"), tr("Ei tulosteta"), Lasku::EITULOSTETA);

    int indeksi =   ui->laskutusCombo->findData(nykyinen);
    if( indeksi < 0) {
        ui->laskutusCombo->setCurrentIndex(0);
    } else {
        ui->laskutusCombo->setCurrentIndex(indeksi);
    }

    paivitaViiteRivi();
    paivitetaanLaskutapoja_ = false;
}

void KantaLaskuDialogi::laskutusTapaMuuttui()
{
    int laskutustapa = ui->laskutusCombo->currentData().toInt();
    if( laskutustapa == Lasku::SAHKOPOSTI)
    {
        ui->valmisNappi->setText( tr("Tallenna ja lähetä sähköpostilla"));
        ui->valmisNappi->setIcon(QIcon(":/pic/email.png"));

    } else if( laskutustapa == Lasku::PDF) {
        ui->valmisNappi->setText( tr("Tallenna ja toimita"));
        ui->valmisNappi->setIcon(QIcon(":/pic/pdf.png"));
    } else if( laskutustapa == Lasku::EITULOSTETA) {
        ui->valmisNappi->setText( tr("Tallenna reskontraan"));
        ui->valmisNappi->setIcon(QIcon(":/pic/ok.png"));
    } else if( laskutustapa == Lasku::POSTITUS){
        ui->valmisNappi->setText( tr("Tallenna ja postita"));
        ui->valmisNappi->setIcon(QIcon(":/pic/mail.png"));
    } else if( laskutustapa == Lasku::VERKKOLASKU) {
        ui->valmisNappi->setText(tr("Tallenna ja lähetä"));
        ui->valmisNappi->setIcon(QIcon(":/pic/verkkolasku.png"));
    } else {
        ui->valmisNappi->setText( tr("Tallenna ja tulosta"));
        ui->valmisNappi->setIcon(QIcon(":/pic/tulosta.png"));
    }

    int saateIndex = ui->tabWidget->indexOf( ui->tabWidget->findChild<QWidget*>("saate") );
    ui->tabWidget->setTabEnabled( saateIndex, laskutustapa == Lasku::SAHKOPOSTI);
}

void KantaLaskuDialogi::maksuTapaMuuttui()
{
    int maksutapa = ui->maksuCombo->currentData().toInt();

    ui->valvontaLabel->setVisible( maksutapa == Lasku::LASKU || maksutapa == Lasku::KUUKAUSITTAINEN);
    ui->valvontaCombo->setVisible( maksutapa == Lasku::LASKU || maksutapa == Lasku::KUUKAUSITTAINEN);
    valvontaMuuttui();


    ui->eraLabel->setVisible( maksutapa != Lasku::KATEINEN );
    ui->eraDate->setVisible( maksutapa != Lasku::KATEINEN && maksutapa != Lasku::KUUKAUSITTAINEN );

    ui->maksuaikaLabel->setVisible( maksutapa != Lasku::KATEINEN && maksutapa != Lasku::KUUKAUSITTAINEN );
    ui->maksuaikaSpin->setVisible( maksutapa != Lasku::KATEINEN && maksutapa != Lasku::KUUKAUSITTAINEN );


    ui->toistoErapaivaSpin->setVisible( maksutapa == Lasku::KUUKAUSITTAINEN);
    ui->viivkorkoLabel->setVisible( maksutapa != Lasku::KATEINEN );
    ui->viivkorkoSpin->setVisible( maksutapa != Lasku::KATEINEN );

    ui->hyvitaEnnakkoNappi->setVisible( maksutapa != Lasku::ENNAKKOLASKU
                                        && tosite()->tyyppi() == TositeTyyppi::MYYNTILASKU &&
                                        ennakkoModel_->rowCount());

    tosite()->rivit()->asetaEnnakkolasku(this->ui->maksuCombo->currentData().toInt() == Lasku::ENNAKKOLASKU);

    ui->toimituspvmLabel->setVisible(maksutapa != Lasku::ENNAKKOLASKU);
    ui->toimitusDate->setVisible(maksutapa != Lasku::ENNAKKOLASKU);
    ui->jaksoViivaLabel->setVisible(maksutapa != Lasku::ENNAKKOLASKU);
    ui->jaksoDate->setVisible( maksutapa != Lasku::ENNAKKOLASKU);

    int toistoIndex = ui->tabWidget->indexOf( ui->tabWidget->findChild<QWidget*>("toisto") );
    ui->tabWidget->setTabEnabled( toistoIndex, maksutapa == Lasku::LASKU || maksutapa == Lasku::KUUKAUSITTAINEN);

    ui->toimituspvmLabel->setText( maksutapa == Lasku::KUUKAUSITTAINEN ? tr("Laskut ajalla") : tr("Toimituspäivä") );

}

void KantaLaskuDialogi::valvontaMuuttui()
{
    const int valvonta = ui->valvontaCombo->isVisible() ? ui->valvontaCombo->currentData().toInt() : Lasku::LASKUVALVONTA;
    ui->tarkeCombo->setVisible( valvonta == Lasku::HUONEISTO || valvonta == Lasku::VAKIOVIITE );
    if( valvonta == Lasku::VAKIOVIITE )
        ui->tarkeCombo->setModel( kp()->vakioViitteet() );
    else if( valvonta == Lasku::HUONEISTO)
        ui->tarkeCombo->setModel( huoneistot_ );

    paivitaViiteRivi();
}

void KantaLaskuDialogi::paivitaViiteRivi()
{
    ViiteNumero viite( tosite()->viite() );
    const int valvonta = ui->valvontaCombo->isVisible() ? ui->valvontaCombo->currentData().toInt() : Lasku::LASKUVALVONTA;
    if( valvonta == Lasku::ASIAKAS && !ladattuAsiakas_.isEmpty()) {
        viite = ViiteNumero(ViiteNumero::ASIAKAS, ladattuAsiakas_.value("id").toInt());
    } else if( valvonta == Lasku::VAKIOVIITE || valvonta == Lasku::HUONEISTO) {
        viite = ViiteNumero( ui->tarkeCombo->currentData(VakioViiteModel::ViiteRooli).toString() );
    }

    ui->viiteLabel->setVisible( viite.tyyppi());
    ui->viiteText->setVisible( viite.tyyppi());
    ui->viiteText->setText(viite.valeilla());
}

void KantaLaskuDialogi::naytaLoki()
{
    NaytinIkkuna *naytin = new NaytinIkkuna();
    QVariant var = ui->lokiView->currentIndex().data(Qt::UserRole);

    QString data = QString::fromUtf8( QJsonDocument::fromVariant(var).toJson(QJsonDocument::Indented) );
    naytin->nayta(data);
}
