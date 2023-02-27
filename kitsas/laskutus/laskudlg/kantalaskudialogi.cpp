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

#include "../ryhmalasku/kielidelegaatti.h"
#include "model/tositerivit.h"
#include "model/tositeloki.h"
#include "model/tositeviennit.h"

#include "naytin/naytinikkuna.h"
#include "../viitenumero.h"

#include "../vakioviite/vakioviitemodel.h"
#include "../huoneisto/huoneistomodel.h"
#include "../tulostus/laskuntulostaja.h"

#include <QJsonDocument>
#include <QSettings>

#include <QPdfWriter>
#include <QPainter>
#include <QMessageBox>
#include <QFileDialog>

#include "rekisteri/maamodel.h"
#include "pilvi/pilvimodel.h"
#include "naytin/naytinikkuna.h"
#include "model/toiminimimodel.h"
#include "model/bannermodel.h"
#include "liite/liitteetmodel.h"

#include "maaritys/verkkolasku/verkkolaskumaaritys.h"

#include <QSortFilterProxyModel>


KantaLaskuDialogi::KantaLaskuDialogi(Tosite *tosite, QWidget *parent) :
    QDialog(parent),
    ui( new Ui::LaskuDialogi),
    tosite_(tosite),
    proxy_(new QSortFilterProxyModel(this))
{
    ui->setupUi(this);
    tosite_->setParent(this);

    setAttribute(Qt::WA_DeleteOnClose);
    resize(800,600);
    restoreGeometry( kp()->settings()->value("LaskuDialogi").toByteArray());

    alustaUi();
    teeConnectit();

    tositteelta();

    ui->liiteView->setModel( tosite->liitteet() );
    ui->liiteView->setDropIndicatorShown(true);
    ui->liiteView->setIconSize(QSize(256,256));
    paivitaLiiteNapit();


}

KantaLaskuDialogi::~KantaLaskuDialogi() {
    kp()->settings()->setValue("LaskuDialogi", saveGeometry());
    delete ui;
}

QString KantaLaskuDialogi::asiakkaanAlvTunnus() const
{
    return ladattuAsiakas_.value("alvtunnus").toString();
}

int KantaLaskuDialogi::maksutapa() const
{
    return ui->maksuCombo->currentData().toInt();
}

void KantaLaskuDialogi::tulosta(QPagedPaintDevice *printer) const
{
    printer->setPageSize( QPageSize(QPageSize::A4));
    printer->setPageMargins( QMarginsF(10,10,10,10), QPageLayout::Millimeter );

    QPainter painter( printer);

    LaskunTulostaja tulostaja( kp());
    Tosite tulostettava;
    tulostettava.lataa(tosite_->tallennettava());

    tulostaja.tulosta(tulostettava, printer, &painter);
    painter.end();
}


void KantaLaskuDialogi::alustaUi()
{
    KieliDelegaatti::alustaKieliCombo(ui->kieliCombo);
    ui->toimitusDate->setNullable(true);
    ui->jaksoDate->setNull();
    ui->tilausPvm->setNull();

    ui->maksuaikaSpin->setValue(kp()->asetukset()->luku(AsetusModel::LaskuMaksuaika,14) );
    ui->saateEdit->setPlainText( kp()->asetukset()->asetus(AsetusModel::EmailSaate) );
    ui->viivkorkoSpin->setValue( kp()->asetukset()->asetus(AsetusModel::LaskuPeruskorko).toDouble() + 7.0 );

    int lokiIndex = ui->tabWidget->indexOf( ui->tabWidget->findChild<QWidget*>("loki") );
    ui->tabWidget->setTabEnabled( lokiIndex, false);

    ui->lokiView->setModel( tosite_->loki() );
    ui->lokiView->horizontalHeader()->setSectionResizeMode(TositeLoki::KAYTTAJA, QHeaderView::Stretch);

    ui->hyvitaEnnakkoNappi->setVisible(false);
    ui->riviTyyppiCombo->setVisible(false);

    paivitaLaskutustavat();
    laskutusTapaMuuttui();
    alustaMaksutavat();
    alustaToiminimiCombo();

    ui->bannerCombo->setModel( kp()->bannerit() );
    ui->bannerCombo->setVisible( ui->bannerCombo->model()->rowCount() > 1 );

    ui->laskuPvm->setDateRange(kp()->tilitpaatetty(), kp()->tilikaudet()->kirjanpitoLoppuu());


}

void KantaLaskuDialogi::alustaToiminimiCombo()
{
    proxy_->setSourceModel( kp()->toiminimet() );
    proxy_->setFilterRole( ToiminimiModel::Nakyva );
    proxy_->setFilterFixedString("X");
    ui->toiminimiCombo->setModel(proxy_);
    ui->toiminimiCombo->setVisible( proxy_->rowCount() > 1 );
}

bool KantaLaskuDialogi::osoiteKunnossa()
{
    return  ladattuAsiakas_.value("nimi").toString().length() > 1 &&
            ladattuAsiakas_.value("osoite").toString().length() > 1 &&
            ladattuAsiakas_.value("postinumero").toString().length() > 1 &&
            ladattuAsiakas_.value("kaupunki").toString().length() > 1;
}

void KantaLaskuDialogi::lataaLoki()
{
    int lokiIndex = ui->tabWidget->indexOf( ui->tabWidget->findChild<QWidget*>("loki") );
    ui->tabWidget->setTabEnabled( lokiIndex, tosite()->loki()->rowCount() );
    const QString& virheviesti = tosite()->loki()->virheViesti();
    if( virheviesti.isEmpty() ) {
        ui->lokiVirheLabel->hide();
    } else {
        ui->lokiVirheLabel->setText(virheviesti);
        ui->tabWidget->setTabIcon(lokiIndex, QIcon(":/pic/info-punainen.png"));
    }

}

void KantaLaskuDialogi::verkkolaskuUrputus(const QString &viesti)
{
    ui->urputusTeksti->setText(viesti);
    ui->urputusKuva->setVisible( !viesti.isEmpty() );
    ui->urputusTeksti->setVisible( !viesti.isEmpty() );
}




void KantaLaskuDialogi::teeConnectit()
{
    connect( ui->asiakas, &AsiakasToimittajaValinta::muuttui, this, &KantaLaskuDialogi::asiakasMuuttui);

    connect( ui->email, &QLineEdit::textChanged, this, &KantaLaskuDialogi::paivitaLaskutustavat);
    connect( ui->osoiteEdit, &QPlainTextEdit::textChanged, this, &KantaLaskuDialogi::paivitaLaskutustavat);
    connect( ui->laskutusCombo, &QComboBox::currentTextChanged, this, &KantaLaskuDialogi::laskutusTapaMuuttui);
    connect( ui->maksuCombo, &QComboBox::currentTextChanged, this, &KantaLaskuDialogi::maksuTapaMuuttui);
    connect( ui->valvontaCombo, &QComboBox::currentTextChanged, this, &KantaLaskuDialogi::valvontaMuuttui);
    connect( ui->tarkeCombo, qOverload<int>(&QComboBox::currentIndexChanged), this, &KantaLaskuDialogi::paivitaViiteRivi);

    connect( ui->laskuPvm, &KpDateEdit::dateChanged, this, &KantaLaskuDialogi::laskeEraPaiva);
    connect( ui->maksuaikaSpin, qOverload<int>(&QSpinBox::valueChanged), this, &KantaLaskuDialogi::laskeEraPaiva );
    connect( ui->eraDate, &KpDateEdit::dateChanged, this, &KantaLaskuDialogi::laskeMaksuaika);

    connect( ui->esikatseluNappi, &QPushButton::clicked, this, &KantaLaskuDialogi::naytaEsikatselu);
    connect( ui->luonnosNappi, &QPushButton::clicked, this, [this] { this->tallenna(Tosite::LUONNOS); } );
    connect( ui->tallennaNappi, &QPushButton::clicked, this, [this] { this->tallenna(Tosite::VALMISLASKU); } );
    connect( ui->valmisNappi, &QPushButton::clicked, this, [this] { this->tallenna(Tosite::LAHETETAAN); } );

    connect( ui->toimitusDate, &KpDateEdit::dateChanged, this, &KantaLaskuDialogi::toimitusPaivaMuuttuu);

    connect( ui->lokiView, &QTableView::clicked, this, &KantaLaskuDialogi::naytaLoki);
    connect( ui->ohjeNappi, &QPushButton::clicked, this, [this] { kp()->ohje( this->ohje() ); });

    connect( ui->kieliCombo, &QComboBox::currentTextChanged, this, &KantaLaskuDialogi::kieliVaihtuu);

    connect( ui->avaaLiiteNappi, &QPushButton::clicked, this, &KantaLaskuDialogi::naytaLiite);
    connect( ui->lisaaLiiteNappi, &QPushButton::clicked, this, &KantaLaskuDialogi::lisaaLiite);
    connect( ui->poistaLiiteNappi, &QPushButton::clicked, this, &KantaLaskuDialogi::poistaLiite);

    connect( ui->liiteView->selectionModel(), &QItemSelectionModel::currentChanged, this, &KantaLaskuDialogi::paivitaLiiteNapit);
    connect( ui->liiteView, &QListView::clicked, this, &KantaLaskuDialogi::paivitaLiiteNapit);

    connect( tosite()->liitteet(), &LiitteetModel::modelReset, this, &KantaLaskuDialogi::paivitaLiiteNapit);
    connect( tosite()->liitteet(), &LiitteetModel::rowsInserted, this, &KantaLaskuDialogi::paivitaLiiteNapit);
    connect( tosite()->liitteet(), &LiitteetModel::rowsRemoved, this, &KantaLaskuDialogi::paivitaLiiteNapit);
}

void KantaLaskuDialogi::alustaMaksutavat()
{
    ui->maksuCombo->clear();
    ui->maksuCombo->addItem(QIcon(":/pic/lasku.png"), tr("Lasku"), Lasku::LASKU);
    if( tosite()->tyyppi() == TositeTyyppi::MYYNTILASKU) {
        ui->maksuCombo->addItem(QIcon(":/pic/kateinen.png"), tr("Käteinen"), Lasku::KATEINEN);
        if(  kp()->asetukset()->onko(AsetusModel::LaskuKorttitili) ) {
            ui->maksuCombo->addItem(QIcon(":/pic/luottokortti.png"), tr("Korttimaksu"), Lasku::KORTTIMAKSU);
        }
        ui->maksuCombo->addItem(QIcon(":/pic/ennakkolasku.png"), tr("Ennakkolasku"), Lasku::ENNAKKOLASKU);
        ui->maksuCombo->addItem(QIcon(":/pic/suorite.png"), tr("Suoriteperusteinen lasku"), Lasku::SUORITEPERUSTE);
        ui->maksuCombo->addItem(QIcon(":/pic/kuu.svg"), tr("Kuukausittainen lasku"), Lasku::KUUKAUSITTAINEN);
    }
}

void KantaLaskuDialogi::paivitaValvonnat()
{
    if(paivitysKaynnissa_) return;
    paivitysKaynnissa_ = true;
    int nykyinen = ui->valvontaCombo->currentData().toInt();
    ui->valvontaCombo->clear();

    if( maksutapa() != Lasku::KUUKAUSITTAINEN)
        ui->valvontaCombo->addItem(QIcon(":/pic/lasku.png"), tr("Yksittäinen lasku"), Lasku::LASKUVALVONTA);

    bool pilvessa = qobject_cast<PilviModel*>(kp()->yhteysModel());


    if( !ladattuAsiakas_.isEmpty() && pilvessa )
        ui->valvontaCombo->addItem(QIcon(":/pic/mies.png"), tr("Asiakas"), Lasku::ASIAKAS);
    if( kp()->huoneistot()->rowCount())
        ui->valvontaCombo->addItem(QIcon(":/pic/talo.png"), tr("Huoneisto"), Lasku::HUONEISTO);
    if( kp()->vakioViitteet()->rowCount())
        ui->valvontaCombo->addItem(QIcon(":/pic/viivakoodi.png"), tr("Vakioviite"), Lasku::VAKIOVIITE);
    ui->valvontaCombo->addItem(QIcon(":/pic/eikaytossa.png"), tr("Valvomaton"), Lasku::VALVOMATON);

    int indeksi = ui->valvontaCombo->findData(nykyinen);
    ui->valvontaCombo->setCurrentIndex(indeksi > -1 ? indeksi : 0);

    paivitysKaynnissa_ = false;
    valvontaMuuttui();
}

void KantaLaskuDialogi::tositteelta()
{
    tositteeltaKaynnissa_ = true;
    const Lasku& lasku = tosite()->constLasku();

    ui->toimitusDate->setDate( lasku.toimituspvm() );
    ui->jaksoDate->setDateRange(lasku.toimituspvm(), QDate());

    ui->laskuPvm->setDate( lasku.laskunpaiva() );
    ui->jaksoDate->setDate( lasku.jaksopvm() );
    ui->eraDate->setDate( tosite()->erapvm() );

    if( tosite()->kumppani()) {
        taytaAsiakasTiedotMapista( tosite()->kumppanimap() );
        ui->asiakas->valitse( tosite()->kumppanimap());
        ui->osoiteEdit->setEnabled( false );
    } else {
        ui->osoiteEdit->setPlainText( lasku.osoite() );
        ui->email->setText( lasku.email() );
    }

    lataaLoki();


    QVariantMap era = tosite()->viennit()->vienti(0).era();
    bool maksettu = !tosite()->viite().isEmpty() &&  (!era.isEmpty() && qAbs( era.value("saldo").toDouble()) < 1e-5);
    ui->maksettuCheck->setVisible(maksettu);
    if( maksettu ) {
        ui->infoLabel->setText( tr("Maksettu "));
        ui->infoLabel->setStyleSheet("color: green;");
    } else {
        for(int i=0; i < tosite()->loki()->rowCount(); i++) {
            const QModelIndex& lokissa = tosite()->loki()->index(i, 0);
            const int tila = lokissa.data(TositeLoki::TilaRooli).toInt();
            const QDateTime aika = lokissa.data(TositeLoki::AikaRooli).toDateTime();

            if( tila == Tosite::TOIMITETTULASKU) {
                ui->infoLabel->setText( tr("Toimitettu %1").arg( aika.toString("dd.MM.yyyy hh.mm") ));
                break;
            } else if( tila == Tosite::LAHETETTYLASKU) {
                ui->infoLabel->setText( tr("Lähetetty %1").arg( aika.toString("dd.MM.yyyy hh.mm") ));
                break;
            }
        }
    }
    paivitaViiteRivi();

    ui->email->setText( lasku.email());

    ui->maksuCombo->setCurrentIndex( ui->maksuCombo->findData(lasku.maksutapa()) );
    ui->laskutusCombo->setCurrentIndex( ui->laskutusCombo->findData( lasku.lahetystapa() ) );
    ui->kieliCombo->setCurrentIndex( ui->kieliCombo->findData( lasku.kieli() ) );    

    ui->valvontaCombo->setCurrentIndex( ui->valvontaCombo->findData( lasku.valvonta() ));
    ui->toiminimiCombo->setCurrentIndex( ui->toiminimiCombo->findData( lasku.toiminimi(), ToiminimiModel::Indeksi ) );
    ui->bannerCombo->setCurrentIndex( ui->bannerCombo->findData( lasku.bannerId(), BannerModel::IdRooli ) );


    ViiteNumero viite( lasku.viite() );
    if( viite.tyyppi() == ViiteNumero::VAKIOVIITE || viite.tyyppi() == ViiteNumero::HUONEISTO ) {
        ui->tarkeCombo->setCurrentIndex( ui->tarkeCombo->findData( viite.viite(), HuoneistoModel::ViiteRooli ) );
    }

    laskeMaksuaika();
    ui->toistoErapaivaSpin->setValue( lasku.toistuvanErapaiva() ? lasku.toistuvanErapaiva() : 4);

    ui->viivkorkoSpin->setValue( lasku.viivastyskorko() );
    ui->asViiteEdit->setText( lasku.asiakasViite());
    ui->otsikkoEdit->setText( lasku.otsikko() );

    ui->tilaajaEdit->setText( lasku.tilaaja());
    ui->myyjaEdit->setText( lasku.myyja());
    ui->tilausPvm->setDate( lasku.tilausPvm());
    ui->tilausnumeroEdit->setText( lasku.tilausNumero());
    ui->huomautusaikaSpin->setValue( lasku.huomautusAika() );
    ui->sopimusNumeroEdit->setText( lasku.sopimusnumero());

    ui->lisatietoEdit->setPlainText( lasku.lisatiedot() );
    ui->erittelyTextEdit->setPlainText( lasku.erittely().join('\n') );

    ui->saateEdit->setPlainText( lasku.saate() );
    ui->saateOtsikkoEdit->setText( lasku.saateOtsikko());    

    tositteeltaKaynnissa_ = false;
    paivitaViiteRivi();
}

void KantaLaskuDialogi::tositteelle()
{
    tosite()->asetaPvm( maksutapa() == Lasku::SUORITEPERUSTE ?
                        ui->toimitusDate->date() :
                        ui->laskuPvm->date());

    tosite()->asetaKumppani( ladattuAsiakas_ );
    tosite()->lasku().setOsoite( ui->osoiteEdit->toPlainText() );
    tosite()->lasku().setEmail( ui->email->text() );

    tosite()->lasku().setMaksutapa( maksutapa() );
    tosite()->lasku().setLahetystapa( ui->laskutusCombo->currentData().toInt());
    tosite()->lasku().setKieli( ui->kieliCombo->currentData().toString() );

    ViiteNumero viite( tosite()->lasku().viite() );
    const int valvonta = ui->valvontaCombo->isVisible() ?  ui->valvontaCombo->currentData().toInt() : Lasku::LASKUVALVONTA;

    tosite()->lasku().setValvonta( valvonta );
    if( valvonta == Lasku::VAKIOVIITE  || valvonta == Lasku::HUONEISTO) {
        QString viitenumero = ui->tarkeCombo->currentData(HuoneistoModel::ViiteRooli).toString();
        viite = ViiteNumero(viitenumero);
    } else if ( valvonta == Lasku::ASIAKAS) {
        viite = ViiteNumero(ViiteNumero::ASIAKAS, ui->asiakas->id() );
    }

    tosite()->asetaViite( viite );
    tosite()->lasku().setViite( viite );

    tosite()->lasku().setLaskunpaiva( ui->laskuPvm->date());
    tosite()->asetaLaskupvm(ui->laskuPvm->date());

    if( maksutapa() != Lasku::ENNAKKOLASKU)
        tosite()->lasku().setToimituspvm( ui->toimitusDate->date() );

    tosite()->lasku().setJaksopvm( ui->jaksoDate->date() );

    tosite()->asetaErapvm(maksutapa() != Lasku::KUUKAUSITTAINEN
                                   ? ui->eraDate->date()
                                   : QDate());
    tosite()->lasku().setErapaiva( tosite()->erapvm() );

    tosite()->lasku().setToistuvanErapaiva( maksutapa() == Lasku::KUUKAUSITTAINEN
                                            ? ui->toistoErapaivaSpin->value()
                                            : 0 );

    tosite()->lasku().setViivastyskorko( ui->viivkorkoSpin->value() );
    tosite()->lasku().setAsiakasViite( ui->asViiteEdit->text());

    tosite()->asetaOtsikko( ui->otsikkoEdit->text() );
    tosite()->lasku().setOtsikko(ui->otsikkoEdit->text() );

    tosite()->lasku().setTilaaja( ui->tilaajaEdit->text() );
    tosite()->lasku().setMyyja( ui->myyjaEdit->text() );
    tosite()->lasku().setTilausPvm( ui->tilausPvm->date() );
    tosite()->lasku().setTilausNumero( ui->tilausnumeroEdit->text());
    tosite()->lasku().setHuomautusAika( ui->huomautusaikaSpin->value());
    tosite()->lasku().setSopimusnumero( ui->sopimusNumeroEdit->text());

    tosite()->lasku().setToiminimi( ui->toiminimiCombo->currentData(ToiminimiModel::Indeksi).toInt() );
    tosite()->lasku().setBannerId( ui->bannerCombo->currentData(BannerModel::IdRooli).toString() );

    tosite()->lasku().setLisatiedot( ui->lisatietoEdit->toPlainText());
    if( ui->erittelyTextEdit->toPlainText().length()) {
        QStringList erittely;
        QString editorista = ui->erittelyTextEdit->toPlainText() + "\n";
        while(!editorista.isEmpty()) {
            int indeksi = editorista.indexOf('\n');
            if( indeksi > 80) {
                erittely.append(editorista.left(80));
                editorista = editorista.mid(80);
            } else {
                erittely.append(editorista.left(indeksi));
                editorista = editorista.mid(indeksi+1);
            }
        }
        tosite()->lasku().setErittely( erittely );
    } else
        tosite()->lasku().setErittely(QStringList());

    tosite()->lasku().setSaate( ui->saateEdit->toPlainText() );
    tosite()->lasku().setSaateOtsikko( ui->saateOtsikkoEdit->text());

    // #1034 Tositetyyppi laskua tallennettaessa
    if( (kp()->asetukset()->onko(AsetusModel::EriSarjaan) || kp()->asetukset()->onko(AsetusModel::KateisSarjaan)) &&
         !tosite()->id()   )
        tosite()->asetaSarja( kp()->tositeTyypit()->sarja( tosite_->tyyppi(), maksutapa() == Lasku::KATEINEN ) ) ;

}

void KantaLaskuDialogi::asiakasMuuttui()
{
    taytaAsiakasTiedotMapista( ui->asiakas->map() );

    const int asiakasId = ui->asiakas->id();
    ui->osoiteEdit->setEnabled( asiakasId == 0);

    if( !asiakasId && asiakasId_) {
        ladattuAsiakas_.clear();
        ui->osoiteEdit->clear();
        ui->email->clear();
        paivitaLaskutustavat();
        emit alvTunnusVaihtui(QString());
    }
    asiakasId_ = asiakasId;
}

void KantaLaskuDialogi::taytaAsiakasTiedot(QVariant *data)
{
    QVariantMap map = data->toMap();
    taytaAsiakasTiedotMapista(map);

}

void KantaLaskuDialogi::taytaAsiakasTiedotMapista(const QVariantMap &map)
{
    ladattuAsiakas_ = map;
    asiakasId_ = ladattuAsiakas_.value("id").toInt();

    ui->osoiteEdit->setPlainText( MaaModel::instanssi()->muotoiltuOsoite(map));

    const QString haettuEmail = map.value("email").toString();

    if( !haettuEmail.isEmpty() || tositteeltaKaynnissa_ ) {
        ui->email->setText( haettuEmail );
    }

    ui->kieliCombo->setCurrentIndex(ui->kieliCombo->findData(map.value("kieli","FI").toString()));

    paivitaLaskutustavat();
    const int laskutapaIndeksi = map.contains("laskutapa") ? ui->laskutusCombo->findData(map.value("laskutapa", -1)) : -1;
    if( laskutapaIndeksi > -1)
        ui->laskutusCombo->setCurrentIndex(laskutapaIndeksi);

    if(  !tosite()->id() || !tositteeltaKaynnissa_ ) {

        if( map.contains("maksuaika")) {
            // Jos on määritelty asiakaskohtainen maksuaika
            // käytetään sitä

            int maksuaika = map.value("maksuaika").toInt();
            ui->maksuaikaSpin->setValue(maksuaika);
            QDate pvm = ui->laskuPvm->date().addDays(maksuaika);
            ui->eraDate->setDate( Lasku::oikaiseErapaiva(pvm) );
        }

        if( map.value("alvtunnus").toString().isEmpty())
            // Yksityishenkilön viivästyskorko on peruskorko + 7 %
            ui->viivkorkoSpin->setValue( kp()->asetukset()->asetus(AsetusModel::LaskuPeruskorko).toDouble() + 7.0 );
        else
            // Yrityksen viivästyskorko on peruskorko + 8 %
            ui->viivkorkoSpin->setValue( kp()->asetukset()->asetus(AsetusModel::LaskuPeruskorko).toDouble() + 8.0 );

    }
    emit alvTunnusVaihtui( asiakkaanAlvTunnus() );
}

QRegularExpression KantaLaskuDialogi::emailRe__ = QRegularExpression(R"(^.*@.*\.\w+$)");

void KantaLaskuDialogi::paivitaLaskutustavat()
{
    if(paivitysKaynnissa_) {
        return;
    }
    paivitysKaynnissa_ = true;
    int nykyinen = ui->laskutusCombo->currentData().toInt();
    ui->laskutusCombo->clear();

    ui->laskutusCombo->addItem( QIcon(":/pic/tulosta.png"), tr("Tulosta lasku"), Lasku::TULOSTETTAVA);


    bool osoitekunnossa = osoiteKunnossa();
    const int finvoiceMoodi = kp()->asetukset()->luku(AsetusModel::FinvoiceKaytossa);
    const QString autentikointitila = kp()->asetukset()->asetus(AsetusModel::MaventaAutentikointiTila);

    // Voidaan postittaa vain jos osoite asiakasrekisterissä
    if( osoitekunnossa )
        ui->laskutusCombo->addItem( QIcon(":/pic/mail.png"), tr("Postita lasku"), Lasku::POSTITUS);


    if( (finvoiceMoodi == VerkkolaskuMaaritys::PAIKALLINEN || finvoiceMoodi == VerkkolaskuMaaritys::MAVENTA) &&
         maksutapa() != Lasku::KATEINEN && maksutapa() != Lasku::KORTTIMAKSU && maksutapa() != Lasku::KUUKAUSITTAINEN &&
         ladattuAsiakas_.value("ovt").toString().length() > 9 && ladattuAsiakas_.value("operaattori").toString().length() > 4) {
        if( !osoitekunnossa ) {
            verkkolaskuUrputus( tr("Ei voi lähettää verkkolaskuna, koska vastaanottajan postiosoite on puutteellinen.") );
        } else if( finvoiceMoodi == VerkkolaskuMaaritys::MAVENTA && autentikointitila != "SIGNED"
                   && autentikointitila != "NONE" && autentikointitila != "VERIFIED" && autentikointitila != "PROFILESOK") {
            verkkolaskuUrputus( tr("Verkkolaskutuksen käyttöönotto on kesken.") );
        } else {
            verkkolaskuUrputus();
            ui->laskutusCombo->addItem( QIcon(":/pic/verkkolasku.png"), tr("Verkkolasku"), Lasku::VERKKOLASKU);
        }

    } else {
        verkkolaskuUrputus();
    }

    if( emailRe__.match( ui->email->text()).hasMatch() )
            ui->laskutusCombo->addItem(QIcon(":/pic/email.png"), tr("Lähetä sähköpostilla"), Lasku::SAHKOPOSTI);

    ui->laskutusCombo->addItem( QIcon(":/pic/pdf.png"), tr("Tallenna pdf-tiedostoon"), Lasku::PDF);
    ui->laskutusCombo->addItem( QIcon(":/pic/tyhja.png"), tr("Ei tulosteta"), Lasku::EITULOSTETA);

    int indeksi =   ui->laskutusCombo->findData(nykyinen);
    if( indeksi < 0) {
        ui->laskutusCombo->setCurrentIndex(0);
    } else {
        ui->laskutusCombo->setCurrentIndex(indeksi);
    }

    laskutusTapaMuuttui();
    KantaLaskuDialogi::maksuTapaMuuttui();
    paivitysKaynnissa_ = false;
    paivitaValvonnat();


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
    ui->tabWidget->setTabEnabled( saateIndex, laskutustapa == Lasku::SAHKOPOSTI || !ui->laskutusCombo->isVisible());
}

void KantaLaskuDialogi::maksuTapaMuuttui()
{
    int maksutapa = ui->maksuCombo->currentData().toInt();

    ui->valvontaLabel->setVisible( maksutapa == Lasku::LASKU || maksutapa == Lasku::KUUKAUSITTAINEN);
    ui->valvontaCombo->setVisible( maksutapa == Lasku::LASKU || maksutapa == Lasku::KUUKAUSITTAINEN);
    valvontaMuuttui();


    ui->eraLabel->setVisible( maksutapa != Lasku::KATEINEN && maksutapa != Lasku::KORTTIMAKSU);
    ui->eraDate->setVisible( maksutapa != Lasku::KATEINEN && maksutapa != Lasku::KORTTIMAKSU && maksutapa != Lasku::KUUKAUSITTAINEN );

    ui->maksuaikaLabel->setVisible( maksutapa != Lasku::KATEINEN && maksutapa != Lasku::KORTTIMAKSU && maksutapa != Lasku::KUUKAUSITTAINEN );
    ui->maksuaikaSpin->setVisible( maksutapa != Lasku::KATEINEN && maksutapa != Lasku::KORTTIMAKSU && maksutapa != Lasku::KUUKAUSITTAINEN );


    ui->toistoErapaivaSpin->setVisible( maksutapa == Lasku::KUUKAUSITTAINEN);
    ui->viivkorkoLabel->setVisible( maksutapa != Lasku::KATEINEN && maksutapa != Lasku::KORTTIMAKSU);
    ui->viivkorkoSpin->setVisible( maksutapa != Lasku::KATEINEN && maksutapa != Lasku::KORTTIMAKSU);

    tosite()->rivit()->asetaEnnakkolasku(this->ui->maksuCombo->currentData().toInt() == Lasku::ENNAKKOLASKU);

    ui->toimituspvmLabel->setVisible(maksutapa != Lasku::ENNAKKOLASKU);
    ui->toimitusDate->setVisible(maksutapa != Lasku::ENNAKKOLASKU);
    ui->jaksoViivaLabel->setVisible(maksutapa != Lasku::ENNAKKOLASKU);
    ui->jaksoDate->setVisible( maksutapa != Lasku::ENNAKKOLASKU);

    ui->toimituspvmLabel->setText( maksutapa == Lasku::KUUKAUSITTAINEN ? tr("Laskut ajalla") : tr("Toimituspäivä") );

    if( maksutapa == Lasku::SUORITEPERUSTE) {
            ui->laskuPvm->setDateRange(QDate(), QDate());
            ui->toimitusDate->setDateRange(kp()->tilitpaatetty(), kp()->tilikaudet()->kirjanpitoLoppuu());
    } else {
            ui->laskuPvm->setDateRange(kp()->tilitpaatetty(), kp()->tilikaudet()->kirjanpitoLoppuu());
            ui->toimitusDate->setDateRange(QDate(), QDate());
    }

    paivitaLaskutustavat();
    paivitaValvonnat();
}

void KantaLaskuDialogi::valvontaMuuttui()
{
    if( paivitysKaynnissa_ ) return;

    bool valvontakaytossa = maksutapa() != Lasku::SUORITEPERUSTE && maksutapa() != Lasku::KATEINEN && maksutapa() != Lasku::KORTTIMAKSU;

    const int valvonta = valvontakaytossa ? ui->valvontaCombo->currentData().toInt() : Lasku::LASKUVALVONTA;
    ui->tarkeCombo->setVisible( valvonta == Lasku::HUONEISTO || valvonta == Lasku::VAKIOVIITE );
    if( valvonta == Lasku::VAKIOVIITE && ui->tarkeCombo->model() != kp()->vakioViitteet())
        ui->tarkeCombo->setModel( kp()->vakioViitteet() );
    else if( valvonta == Lasku::HUONEISTO && ui->tarkeCombo->model() != kp()->huoneistot())
        ui->tarkeCombo->setModel( kp()->huoneistot() );

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

    ui->rivitView->setColumnHidden(TositeRivit::TILI, valvonta == Lasku::VAKIOVIITE);

    ui->viiteText->setVisible( viite.tyyppi());

    ui->eiViitettaLabel->setVisible(  viite.tyyppi() == ViiteNumero::VIRHEELLINEN );
    ui->viiteText->setText(viite.valeilla());

    setWindowTitle(otsikko());
}

void KantaLaskuDialogi::laskeEraPaiva()
{
    if( paivitysKaynnissa_ ) return;
    paivitysKaynnissa_ = true;

    QDate pvm = ui->laskuPvm->date().addDays(ui->maksuaikaSpin->value());
    ui->eraDate->setDate( Lasku::oikaiseErapaiva(pvm) );

    paivitysKaynnissa_ = false;
}

void KantaLaskuDialogi::laskeMaksuaika()
{
    if( paivitysKaynnissa_ ) return;
    paivitysKaynnissa_ = true;

    ui->maksuaikaSpin->setValue( ui->laskuPvm->date().daysTo( ui->eraDate->date() ) );

    paivitysKaynnissa_ = false;
}

void KantaLaskuDialogi::naytaLoki()
{
    NaytinIkkuna *naytin = new NaytinIkkuna();
    QVariant var = ui->lokiView->currentIndex().data(Qt::UserRole);

    QString data = QString::fromUtf8( QJsonDocument::fromVariant(var).toJson(QJsonDocument::Indented) );
    naytin->nayta(data);
}

void KantaLaskuDialogi::naytaEsikatselu()
{
    tositteelle();
    esikatsele();
}

bool KantaLaskuDialogi::tarkasta()
{
    const QDate& pvm = paivamaara();
    if( pvm <= kp()->tilitpaatetty() ) {
        QMessageBox::critical(this, tr("Lukittu tilikausi"), maksutapa() == Lasku::SUORITEPERUSTE ? tr("Toimituspäivämäärälle ei ole avointa tilikautta") : tr("Laskun päivämäärälle ei ole avointa tilikautta"));
        return false;
    }

    if( pvm > kp()->tilikaudet()->kirjanpitoLoppuu()) {
        QMessageBox::critical(this, tr("Puuttuva tilikausi"), maksutapa() == Lasku::SUORITEPERUSTE ? tr("Toimituspäivämäärälle ei ole avointa tilikautta") : tr("Laskun päivämäärälle ei ole avointa tilikautta"));
        return false;
    }


    if( ui->jaksoDate->isInvalid() ) {
        QMessageBox::critical(this, tr("Virheellinen toimitusjakso"), tr("Toimitusjakson päättymispäivä on virheellinen."));
        return false;
    }

    return true;
}

void KantaLaskuDialogi::salliTallennus(bool sallinta)
{
    bool laskuJoTallennettu = tosite()->tila() >= Tosite::KIRJANPIDOSSA;

    ui->valmisNappi->setEnabled(sallinta && kp()->yhteysModel()->onkoOikeutta(YhteysModel::LASKU_LAHETTAMINEN));
    ui->tallennaNappi->setEnabled(sallinta &&  tosite()->tila() < Tosite::LAHETETTYLASKU  &&
                                  kp()->yhteysModel()->onkoOikeutta(YhteysModel::LASKU_LAATIMINEN));
    ui->luonnosNappi->setEnabled( !laskuJoTallennettu && kp()->yhteysModel()->onkoOikeutta(YhteysModel::LASKU_LAATIMINEN));
}

QDate KantaLaskuDialogi::paivamaara() const
{
    if( maksutapa() == Lasku::SUORITEPERUSTE)
        return ui->toimitusDate->date();
    else
        return ui->laskuPvm->date();
}

void KantaLaskuDialogi::toimitusPaivaMuuttuu(const QDate &pvm)
{
    ui->jaksoDate->setEnabled(pvm.isValid());
    if( pvm.isValid())
        ui->jaksoDate->setDateRange( pvm, QDate() );
}

void KantaLaskuDialogi::kieliVaihtuu()
{
    QStringList kielet;
    kielet << "fi" << "en" << "sv";

    const Monikielinen saateOtsikko( kp()->asetukset()->asetus("Laskuteksti/Saate_otsikko") );
    const Monikielinen saate( kp()->asetukset()->asetus("Laskuteksti/Saate"));
    const Monikielinen lisatiedot( kp()->asetukset()->asetus("Laskuteksti/Lisatiedot"));

    const QString nykykieli = ui->kieliCombo->currentData().toString().toLower();

    for( auto & kieli : kielet) {
        if( saateOtsikko.kaannos(kieli) == ui->saateOtsikkoEdit->text() &&
            saate.kaannos(kieli) == ui->saateEdit->toPlainText() &&
            lisatiedot.kaannos(kieli) == ui->lisatietoEdit->toPlainText() ) {
            ui->saateOtsikkoEdit->setText( saateOtsikko.kaannos(nykykieli) );
            ui->saateEdit->setPlainText( saate.kaannos(nykykieli) );
            ui->lisatietoEdit->setPlainText( lisatiedot.kaannos(nykykieli));
            return;
        }
    }
}

void KantaLaskuDialogi::naytaLiite()
{
    QByteArray data = ui->liiteView->currentIndex().data(LiitteetModel::SisaltoRooli).toByteArray();
    if( data.isEmpty()) {
        const int liiteId = ui->liiteView->currentIndex().data(LiitteetModel::IdRooli).toInt();
        NaytinIkkuna::naytaLiite(liiteId);
    } else {
        NaytinIkkuna::nayta(data);
    }
}

void KantaLaskuDialogi::lisaaLiite()
{
    QString polku = QFileDialog::getOpenFileName(this, tr("Lisää liite"),QString(),tr("Pdf-tiedosto (*.pdf);;Kuvat (*.png *.jpg);;CSV-tiedosto (*.csv);;Kaikki tiedostot (*.*)"));
    if( !polku.isEmpty()) {
        tosite()->liitteet()->lisaaHetiTiedosto(polku);
    }
}

void KantaLaskuDialogi::poistaLiite()
{
    if( ui->liiteView->currentIndex().isValid() )
    {
        if( QMessageBox::question(this, tr("Poista liite"),
                                  tr("Poistetaanko liite %1. Poistettua liitettä ei voi palauttaa!").arg( ui->liiteView->currentIndex().data(Qt::DisplayRole).toString()),
                                  QMessageBox::Yes, QMessageBox::Cancel) == QMessageBox::Yes )
        {
            ui->poistaLiiteNappi->setEnabled(false);
            tosite()->liitteet()->poista( ui->liiteView->currentIndex().row() );
        }
    }
}

void KantaLaskuDialogi::paivitaLiiteNapit()
{
    bool valittu = ui->liiteView->currentIndex().isValid();
    // Laskun kuvaa ei voi poistaa
    ui->poistaLiiteNappi->setEnabled( valittu &&
                                      ui->liiteView->currentIndex().data(LiitteetModel::RooliRooli).toString().isEmpty());
    ui->avaaLiiteNappi->setEnabled(valittu);

    const int tabIndex = ui->tabWidget->indexOf( ui->tabWidget->findChild<QWidget*>("liitteet") );
    if( tosite()->liitteet()->rowCount())
        ui->tabWidget->setTabIcon( tabIndex , QIcon(":/pic/liite-aktiivinen.png"));
    else
        ui->tabWidget->setTabIcon( tabIndex, QIcon(":/pic/liite"));

}

QString KantaLaskuDialogi::otsikko() const
{
    return tr("Lasku %1").arg(tosite_->lasku().numero());
}
