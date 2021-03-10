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
#include "laskudialogi.h"
#include "ui_laskudialogi.h"

#include "db/kirjanpito.h"
#include "db/tositetyyppimodel.h"

#include "ennakkohyvitysmodel.h"
#include "ryhmalasku/kielidelegaatti.h"
#include "laskurivitmodel.h"
#include "model/tositeloki.h"
#include "tuotedialogi.h"

#include "laskutusverodelegaatti.h"
#include "kirjaus/eurodelegaatti.h"
#include "kirjaus/kohdennusdelegaatti.h"
#include "kirjaus/tilidelegaatti.h"

#include "naytin/naytinikkuna.h"
#include "viitenumero.h"

#include <QJsonDocument>
#include <QMenu>
#include <QSettings>
#include <QSortFilterProxyModel>


LaskuDialogi::LaskuDialogi(QWidget *parent) :
    QDialog(parent), ui( new Ui::LaskuDialogi),
    ennakkoModel_(new EnnakkoHyvitysModel(this))
{
    ui->setupUi(this);

    setAttribute(Qt::WA_DeleteOnClose);
    resize(800,600);
    restoreGeometry( kp()->settings()->value("LaskuDialogi").toByteArray());

    alustaUi();
    teeConnectit();

}

LaskuDialogi::~LaskuDialogi() {
    kp()->settings()->setValue("LaskuDialogi", saveGeometry());
    delete ui;
}

void LaskuDialogi::lataa(const QVariantMap &data)
{
    QVariant variant(data);
    lasku_.lataaData(&variant);
}

void LaskuDialogi::lataa(const int tositeId)
{
    lasku_.lataa(tositeId);
}

void LaskuDialogi::asetaAsiakas(const int asiakas)
{
    ui->asiakas->set(asiakas);
}

void LaskuDialogi::alustaUi()
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

    ui->lokiView->setModel(lasku_.loki());
    poistaLiikaTab();

    paivitaLaskutustavat();
    alustaMaksutavat();
    alustaValvonta();
}

void LaskuDialogi::alustaRivitTab()
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
             this, &LaskuDialogi::tuotteidenKonteksiValikko);


    ui->rivitView->setModel(lasku_.rivit());

    ui->rivitView->horizontalHeader()->setSectionResizeMode(LaskuRivitModel::NIMIKE, QHeaderView::Stretch);
    ui->rivitView->setItemDelegateForColumn(LaskuRivitModel::AHINTA, new EuroDelegaatti());
    ui->rivitView->setItemDelegateForColumn(LaskuRivitModel::TILI, new TiliDelegaatti());

    KohdennusDelegaatti *kohdennusDelegaatti = new KohdennusDelegaatti(this);
    kohdennusDelegaatti->asetaKohdennusPaiva(ui->toimitusDate->date());
    ui->rivitView->setItemDelegateForColumn(LaskuRivitModel::KOHDENNUS, kohdennusDelegaatti );

    connect( ui->toimitusDate , SIGNAL(dateChanged(QDate)), kohdennusDelegaatti, SLOT(asetaKohdennusPaiva(QDate)));
    connect( ui->tuoteFiltterinEditori, &QLineEdit::textChanged, proxy, &QSortFilterProxyModel::setFilterFixedString);

    ui->rivitView->setItemDelegateForColumn(LaskuRivitModel::BRUTTOSUMMA, new EuroDelegaatti());
    ui->rivitView->setItemDelegateForColumn(LaskuRivitModel::ALV, new LaskutusVeroDelegaatti(this));

    ui->rivitView->setColumnHidden( LaskuRivitModel::ALV, !kp()->asetukset()->onko("AlvVelvollinen") );
    ui->rivitView->setColumnHidden( LaskuRivitModel::KOHDENNUS, !kp()->kohdennukset()->kohdennuksia());

    connect( ui->uusituoteNappi, &QPushButton::clicked, [this] { (new TuoteDialogi(this))->uusi(); } );
    connect( ui->lisaaRiviNappi, &QPushButton::clicked, [this] { this->lasku_.rivit()->lisaaRivi();} );
    connect( ui->poistaRiviNappi, &QPushButton::clicked, [this] {
        if( this->ui->rivitView->currentIndex().isValid())
                this->lasku_.rivit()->poistaRivi( ui->rivitView->currentIndex().row());
    });

    ui->splitter->setStretchFactor(0,1);
    ui->splitter->setStretchFactor(1,3);

    connect( ui->tuoteView, &QTableView::clicked, [this] (const QModelIndex& index)
        { this->lasku_.rivit()->lisaaRivi( index.data(TuoteModel::TuoteMapRooli).toMap() ); }  );
}

void LaskuDialogi::poistaLiikaTab()
{
    ui->tabWidget->removeTab( ui->tabWidget->indexOf( ui->tabWidget->findChild<QWidget*>("maksumuistutus") ) );
}

void LaskuDialogi::teeConnectit()
{
    connect( &lasku_, &Lasku::ladattu, this, &LaskuDialogi::tositteelta);
    connect( ui->asiakas, &AsiakasToimittajaValinta::muuttui, this, &LaskuDialogi::asiakasMuuttui);
    connect( ui->asiakas, &AsiakasToimittajaValinta::valittu, this, &LaskuDialogi::asiakasMuuttui);

    connect( ui->email, &QLineEdit::textChanged, this, &LaskuDialogi::paivitaLaskutustavat);
    connect( ui->osoiteEdit, &QPlainTextEdit::textChanged, this, &LaskuDialogi::paivitaLaskutustavat);
    connect( ui->laskutusCombo, &QComboBox::currentTextChanged, this, &LaskuDialogi::laskutusTapaMuuttui);
    connect( ui->maksuCombo, &QComboBox::currentTextChanged, this, &LaskuDialogi::maksuTapaMuuttui);
    connect( ui->valvontaCombo, &QComboBox::currentTextChanged, this, &LaskuDialogi::valvontaMuuttui);
}

void LaskuDialogi::alustaMaksutavat()
{
    ui->maksuCombo->clear();
    ui->maksuCombo->addItem(QIcon(":/pic/lasku.png"), tr("Lasku"), Lasku::LASKU);
    if( lasku_.tyyppi() == TositeTyyppi::MYYNTILASKU) {
        ui->maksuCombo->addItem(QIcon(":/pic/kateinen.png"), tr("Käteinen"), Lasku::KATEINEN);
        ui->maksuCombo->addItem(QIcon(":/pic/ennakkolasku.png"), tr("Ennakkolasku"), Lasku::ENNAKKOLASKU);
        ui->maksuCombo->addItem(QIcon(":/pic/suorite.png"), tr("Suoriteperusteinen lasku"), Lasku::SUORITEPERUSTE);

        ui->maksuCombo->addItem(QIcon(":/pic/refresh.png"), tr("Kuukausittainen lasku"), Lasku::KUUKAUSITTAINEN);
    }
}

void LaskuDialogi::alustaValvonta()
{
    ui->valvontaCombo->addItem(QIcon(":/pic/lasku.png"), tr("Yksittäinen lasku"), Lasku::LASKUVALVONTA);
    ui->valvontaCombo->addItem(QIcon(":/pic/mies.png"), tr("Asiakas"), Lasku::ASIAKAS);
    ui->valvontaCombo->addItem(QIcon(":/pic/talo.png"), tr("Huoneisto"), Lasku::HUONEISTO);
    ui->valvontaCombo->addItem(QIcon(":/pic/viivakoodi.png"), tr("Vakioviite"), Lasku::VAKIO);
    ui->valvontaCombo->addItem(QIcon(":/pic/eikaytossa.png"), tr("Valvomaton"), Lasku::VALVOMATON);
}

void LaskuDialogi::tuotteidenKonteksiValikko(QPoint pos)
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

void LaskuDialogi::tositteelta()
{
    ui->asiakas->set( lasku_.kumppani(), lasku_.kumppaninimi() );

    ui->osoiteEdit->setPlainText(lasku_.osoite());
    ui->email->setText( lasku_.email());


    int lokiIndex = ui->tabWidget->indexOf( ui->tabWidget->findChild<QWidget*>("loki") );
    ui->tabWidget->setTabEnabled( lokiIndex, lasku_.loki()->rowCount() );

}

void LaskuDialogi::asiakasMuuttui()
{
    const int asiakasId = ui->asiakas->id();
    ui->osoiteEdit->setEnabled( asiakasId == 0);
    if( asiakasId && asiakasId != ladattuAsiakas_.value("id").toInt()) {
        KpKysely *kysely = kpk( QString("/kumppanit/%1").arg(asiakasId) );
        connect( kysely, &KpKysely::vastaus, this, &LaskuDialogi::taytaAsiakasTiedot);
        kysely->kysy();
    }
    ladattuAsiakas_.clear();
}

void LaskuDialogi::taytaAsiakasTiedot(QVariant *data)
{
    QVariantMap map = data->toMap();
    ladattuAsiakas_ = map;

    ui->osoiteEdit->setPlainText( map.value("nimi").toString() + "\n" +
                                  map.value("osoite").toString() + "\n" +
                                  map.value("postinumero").toString() + " " + map.value("kaupunki").toString());

    ui->email->setText( map.value("email").toString());
    ui->kieliCombo->setCurrentIndex(ui->kieliCombo->findData(map.value("kieli","FI").toString()));

    int id = map.value("id").toInt();

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

void LaskuDialogi::paivitaLaskutustavat()
{
    if(paivitetaanLaskutapoja_) {
        return;
    }
    paivitetaanLaskutapoja_ = true;
    int nykyinen = ui->laskutusCombo->currentData().toInt();
    ui->laskutusCombo->clear();

    QRegularExpression postiRe("(.*\\w.*\n){2,}(\\d{5})\\s(.+)", QRegularExpression::MultilineOption);

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
    paivitetaanLaskutapoja_ = false;
}

void LaskuDialogi::laskutusTapaMuuttui()
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

void LaskuDialogi::maksuTapaMuuttui()
{
    int maksutapa = ui->maksuCombo->currentData().toInt();

    ui->valvontaLabel->setVisible( maksutapa == Lasku::LASKU );
    ui->valvontaCombo->setVisible( maksutapa == Lasku::LASKU );
    valvontaMuuttui();


    ui->eraLabel->setVisible( maksutapa != Lasku::KATEINEN );
    ui->eraDate->setVisible( maksutapa != Lasku::KATEINEN );
    ui->viivkorkoLabel->setVisible( maksutapa != Lasku::KATEINEN );
    ui->viivkorkoSpin->setVisible( maksutapa != Lasku::KATEINEN );

    ui->hyvitaEnnakkoNappi->setVisible( maksutapa != Lasku::ENNAKKOLASKU
                                        && lasku_.tyyppi() == TositeTyyppi::MYYNTILASKU &&
                                        ennakkoModel_->rowCount());

    lasku_.rivit()->asetaEnnakkolasku(this->ui->maksuCombo->currentData().toInt() == Lasku::ENNAKKOLASKU);

    ui->toimituspvmLabel->setVisible(maksutapa != Lasku::ENNAKKOLASKU);
    ui->toimitusDate->setVisible(maksutapa != Lasku::ENNAKKOLASKU);
    ui->jaksoViivaLabel->setVisible(maksutapa != Lasku::ENNAKKOLASKU);
    ui->jaksoDate->setVisible( maksutapa != Lasku::ENNAKKOLASKU);
}

void LaskuDialogi::valvontaMuuttui()
{
    const int valvonta = ui->valvontaCombo->isVisible() ? ui->valvontaCombo->currentData().toInt() : Lasku::LASKUVALVONTA;
    ui->tarkeCombo->setVisible( valvonta == Lasku::HUONEISTO || valvonta == Lasku::VAKIO );
    paivitaViiteRivi();
}

void LaskuDialogi::paivitaViiteRivi()
{
    ViiteNumero viite( lasku_.viite() );
    const int valvonta = ui->valvontaCombo->isVisible() ? ui->valvontaCombo->currentData().toInt() : Lasku::LASKUVALVONTA;
    if( valvonta == !Lasku::ASIAKAS && ladattuAsiakas_.isEmpty()) {
        viite = ViiteNumero(ViiteNumero::ASIAKAS, ladattuAsiakas_.value("id").toInt());
    }

    ui->viiteLabel->setVisible( viite.tyyppi());
    ui->viiteText->setVisible( viite.tyyppi());
    ui->viiteText->setText(viite.valeilla());
}

void LaskuDialogi::naytaLoki()
{
    NaytinIkkuna *naytin = new NaytinIkkuna();
    QVariant var = ui->lokiView->currentIndex().data(Qt::UserRole);

    QString data = QString::fromUtf8( QJsonDocument::fromVariant(var).toJson(QJsonDocument::Indented) );
    naytin->nayta(data);
}
