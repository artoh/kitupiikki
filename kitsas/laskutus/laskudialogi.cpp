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


#include "db/kirjanpito.h"

#include "laskudialogi.h"
#include "ui_laskudialogi.h"
#include "laskutusverodelegaatti.h"
#include "ryhmantuontidlg.h"

#include "kirjaus/eurodelegaatti.h"
#include "kirjaus/kohdennusdelegaatti.h"
#include "kirjaus/tilidelegaatti.h"

#include "naytin/naytinikkuna.h"
#include "naytin/naytinview.h"
#include "validator/ytunnusvalidator.h"
#include "asiakkaatmodel.h"
#include "alv/alvilmoitustenmodel.h"

#include "validator/ytunnusvalidator.h"

#include "laskurivitmodel.h"
#include "model/tositevienti.h"

#include "myyntilaskuntulostaja.h"
#include "model/tosite.h"

#include "db/tositetyyppimodel.h"

#include "myyntilaskujentoimittaja.h"
#include "ryhmalasku/ryhmalaskutab.h"
#include "ryhmalasku/laskutettavatmodel.h"
#include "ryhmalasku/kielidelegaatti.h"
#include "ennakkohyvitysmodel.h"
#include "ennakkohyvitysdialogi.h"
#include "tuotedialogi.h"
#include "model/tositeloki.h"

#include "db/yhteysmodel.h"

#include <QDebug>

#include <QPrinter>
#include <QPrintDialog>
#include <QDesktopServices>
#include <QCompleter>
#include <QSqlQueryModel>
#include <QSqlQuery>
#include <QRegExp>
#include <QMessageBox>
#include <QFileDialog>
#include <QMenu>
#include <QAction>
#include <QPrintPreviewDialog>

#include <QSettings>
#include <QRegularExpression>

#include <QMessageBox>
#include <QPdfWriter>
#include <QSplitter>

#include <QTableView>
#include <QHeaderView>
#include <QPainter>
#include <QJsonDocument>

LaskuDialogi::LaskuDialogi(const QVariantMap& data, bool ryhmalasku, int asiakas) :
    rivit_(new LaskuRivitModel(this, data.value("rivit").toList())),
    ui( new Ui::LaskuDialogi),
    ryhmalasku_(ryhmalasku),
    ennakkoModel_(new EnnakkoHyvitysModel(this))
{
    ui->setupUi(this);

    setAttribute(Qt::WA_DeleteOnClose);
    resize(800,600);
    restoreGeometry( kp()->settings()->value("LaskuDialogi").toByteArray());

    KieliDelegaatti::alustaKieliCombo(ui->kieliCombo);

    connect( ui->esikatseluNappi, &QPushButton::clicked, [this] { this->esikatsele(); });

    connect( rivit_, &LaskuRivitModel::dataChanged, this, &LaskuDialogi::paivitaSumma);
    connect( rivit_, &LaskuRivitModel::rowsInserted, this, &LaskuDialogi::paivitaSumma);
    connect( rivit_, &LaskuRivitModel::rowsRemoved, this, &LaskuDialogi::paivitaSumma);
    connect( rivit_, &LaskuRivitModel::modelReset, this, &LaskuDialogi::paivitaSumma);

    alustaRiviTab();
    connect( ui->email, &QLineEdit::textChanged, this, &LaskuDialogi::paivitaLaskutustavat);
    connect( ui->osoiteEdit, &QPlainTextEdit::textChanged, this, &LaskuDialogi::paivitaLaskutustavat);
    connect( ui->laskutusCombo, &QComboBox::currentTextChanged, this, &LaskuDialogi::laskutusTapaMuuttui);
    connect( ui->maksuCombo, &QComboBox::currentTextChanged, this, &LaskuDialogi::maksuTapaMuuttui);

    connect( ui->luonnosNappi, &QPushButton::clicked, [this] () { this->tallenna(Tosite::LUONNOS); });
    connect( ui->tallennaNappi, &QPushButton::clicked, [this] () { this->tallenna(Tosite::VALMISLASKU);});
    connect( ui->valmisNappi, &QPushButton::clicked, [this] () { this->tallenna(Tosite::LAHETETAAN);});

    connect( ui->hyvitaEnnakkoNappi, &QPushButton::clicked, [this] { EnnakkoHyvitysDialogi *dlg = new EnnakkoHyvitysDialogi(this, this->ennakkoModel_); dlg->show(); });
    connect( ennakkoModel_, &EnnakkoHyvitysModel::modelReset, this, &LaskuDialogi::maksuTapaMuuttui);
    connect( ui->ohjeNappi, &QPushButton::clicked, this, &LaskuDialogi::ohje);

    connect( ui->mmMuistutusCheck, &QCheckBox::clicked, this, &LaskuDialogi::paivitaSumma);
    connect( ui->mmMuistutusMaara, &KpEuroEdit::textChanged, this, &LaskuDialogi::paivitaSumma);
    connect( ui->mmViivastysCheck, &QCheckBox::clicked, this, &LaskuDialogi::paivitaSumma);
    connect( ui->mmViivastysAlkaa, &KpDateEdit::dateChanged, this, &LaskuDialogi::paivitaSumma);
    connect( ui->mmViivastysLoppuu, &KpDateEdit::dateChanged, this, &LaskuDialogi::paivitaSumma);

    paivitaLaskutustavat();
    ui->jaksoDate->setNull();    

    ui->viiteLabel->hide();
    ui->viiteText->hide();
    ui->maksettuCheck->hide();
    ui->infoLabel->hide();

    if( !data.isEmpty())
        lataa(data);
    else {
        ui->eraDate->setDate( MyyntiLaskunTulostaja::erapaiva() );
        alustaMaksutavat();
        ui->saateEdit->setPlainText( kp()->asetus("EmailSaate") );
        ui->viivkorkoSpin->setValue( kp()->asetus("LaskuPeruskorko").toDouble() + 7.0 );        
        ui->tabWidget->removeTab( ui->tabWidget->indexOf( ui->tabWidget->findChild<QWidget*>("loki") ) );
    }

    if( tyyppi_ == TositeTyyppi::MAKSUMUISTUTUS) {
        ui->tabWidget->removeTab( ui->tabWidget->indexOf( ui->tabWidget->findChild<QWidget*>("rivit") ) );
    } else {
        ui->tabWidget->removeTab( ui->tabWidget->indexOf( ui->tabWidget->findChild<QWidget*>("maksumuistutus") ) );
    }

    connect( ui->asiakas, &AsiakasToimittajaValinta::valittu, this, &LaskuDialogi::asiakasValittu);

    if(asiakas) {
        ui->asiakas->set(asiakas);
    }

    if( ryhmalasku )
        alustaRyhmalasku();

}


LaskuDialogi::~LaskuDialogi()
{
    kp()->settings()->setValue("LaskuDialogi", saveGeometry());
    delete ui;
}

void LaskuDialogi::paivitaSumma()
{
    if( tyyppi() == TositeTyyppi::MAKSUMUISTUTUS) {
        qlonglong viivastyskorko = laskeViivastysKorko();
        qlonglong summa = viivastyskorko + qRound64( aiempiSaldo_ * 100 ) +
                qRound64( ui->mmMuistutusCheck->isChecked() ?  ui->mmMuistutusMaara->value() * 100 : 0);
        ui->mmViivastysMaara->setText( QString("%L1 €").arg( viivastyskorko / 100.0,0,'f',2));
        ui->summaLabel->setText( QString("%L1 €").arg( summa / 100.0,0,'f',2) );
        ui->mmYhteensa->setText( QString("%L1 €").arg( summa / 100.0,0,'f',2) );
    } else {
        ui->summaLabel->setText( QString("%L1 €").arg( rivit_->yhteensa(),0,'f',2) );        
    }
    paivitaNapit();
}

void LaskuDialogi::paivitaNapit()
{
    bool tallennettavaa = (!rivit_->onkoTyhja() || tyyppi() == TositeTyyppi::MAKSUMUISTUTUS ) &&
            (!ryhmalasku_ || ryhmalaskuTab_->model()->rowCount() );

    ui->luonnosNappi->setEnabled( tallennettavaa && laskunnumero_ == 0);
    ui->tallennaNappi->setEnabled( tallennettavaa );
    ui->valmisNappi->setEnabled( tallennettavaa );

    ui->luonnosNappi->setVisible( kp()->yhteysModel()->onkoOikeutta(YhteysModel::LASKU_LAATIMINEN) );
    ui->tallennaNappi->setVisible( kp()->yhteysModel()->onkoOikeutta(YhteysModel::LASKU_LAATIMINEN));
    ui->valmisNappi->setVisible( kp()->yhteysModel()->onkoOikeutta(YhteysModel::LASKU_LAHETTAMINEN) && !ryhmalasku_);
}



void LaskuDialogi::tulosta(QPagedPaintDevice *printer) const
{
    printer->setPageSize( QPdfWriter::A4);
    printer->setPageMargins( QMarginsF(10,10,10,10), QPageLayout::Millimeter );

    QPainter painter( printer);
    MyyntiLaskunTulostaja::tulosta( data(), printer, &painter, true );

    painter.end();
}

QString LaskuDialogi::otsikko() const
{
    return tr("Lasku");
}





void LaskuDialogi::lisaaTuote()
{
    TuoteDialogi* dialog = new TuoteDialogi(this);
    dialog->uusi();
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



void LaskuDialogi::tulostaLasku()
{

    QPageLayout vanhaleiska = kp()->printer()->pageLayout();
    QPageLayout uusileiska = vanhaleiska;
    uusileiska.setUnits(QPageLayout::Millimeter);
    uusileiska.setMargins( QMarginsF(5.0,5.0,5.0,5.0));
    kp()->printer()->setPageLayout(uusileiska);

    QPrintDialog printDialog( kp()->printer(), this );    
    if( printDialog.exec())
    {
       tulosta( kp()->printer() );
    }

    kp()->printer()->setPageLayout(vanhaleiska);
}


void LaskuDialogi::asiakasValittu(int asiakasId)
{
    KpKysely *kysely = kpk( QString("/kumppanit/%1").arg(asiakasId) );
    connect( kysely, &KpKysely::vastaus, this, &LaskuDialogi::taytaAsiakasTiedot);
    kysely->kysy();
    ennakkoModel_->lataaErat(asiakasId);
}

void LaskuDialogi::taytaAsiakasTiedot(QVariant *data)
{
    QVariantMap map = data->toMap();
    ui->osoiteEdit->setPlainText( map.value("nimi").toString() + "\n" +
                                  map.value("osoite").toString() + "\n" +
                                  map.value("postinumero").toString() + " " + map.value("kaupunki").toString());

    if( !map.value("email").toString().isEmpty())
        ui->email->setText( map.value("email").toString());

    ui->kieliCombo->setCurrentIndex(ui->kieliCombo->findData(map.value("kieli","FI").toString()));

    verkkolaskutettava_ =  kp()->asetukset()->luku("FinvoiceKaytossa") &&
            map.value("ovt").toString().length() > 9 &&
            map.value("operaattori").toString().length() > 4;

    int id = map.value("id").toInt();

    paivitaLaskutustavat();

    if( id != asiakasId_) {
        ui->laskutusCombo->setCurrentIndex(ui->laskutusCombo->findData(map.value("laskutapa", LaskuDialogi::TULOSTETTAVA)));
        asiakasId_ = id;
    }

    if( map.contains("maksuaika")) {
        ui->eraDate->setDate(kp()->paivamaara().addDays(map.value("maksuaika").toInt()));
    }

    asAlvTunnus_ = map.value("alvtunnus").toString();
    if( asAlvTunnus_.isEmpty())
        // Yksityishenkilön viivästyskorko on peruskorko + 7 %
        ui->viivkorkoSpin->setValue( kp()->asetus("LaskuPeruskorko").toDouble() + 7.0 );
    else
        // Yrityksen viivästyskorko on peruskorko + 8 %
        ui->viivkorkoSpin->setValue( kp()->asetus("LaskuPeruskorko").toDouble() + 8.0 );

}

void LaskuDialogi::asiakasHaettuLadattaessa(QVariant *data)
{
    QVariantMap map = data->toMap();
    verkkolaskutettava_ =  kp()->asetukset()->luku("FinvoiceKaytossa") &&
            map.value("ovt").toString().length() > 9 &&
            map.value("operaattori").toString().length() > 4;
    paivitaLaskutustavat();
    if( ui->laskutusCombo->currentData().toInt() == VERKKOLASKU) {
        ui->osoiteEdit->setPlainText(
                    map.value("nimi").toString() + "\n" + map.value("osoite").toString() + "\n" + map.value("postinumero").toString() + " " + map.value("kaupunki").toString() );
    }
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

    ui->laskutusCombo->addItem( QIcon(":/pic/tulosta.png"), tr("Tulosta lasku"), TULOSTETTAVA);
    if( postiRe.match(ui->osoiteEdit->toPlainText()).hasMatch() )
        ui->laskutusCombo->addItem( QIcon(":/pic/mail.png"), tr("Postita lasku"), POSTITUS);
    if( verkkolaskutettava_)
        ui->laskutusCombo->addItem( QIcon(":/pic/verkkolasku.png"), tr("Verkkolasku"), VERKKOLASKU);

    QRegularExpression emailRe(R"(^.*@.*\.\w+$)");
    if( emailRe.match( ui->email->text()).hasMatch() )
            ui->laskutusCombo->addItem(QIcon(":/pic/email.png"), tr("Lähetä sähköpostilla"), SAHKOPOSTI);
    ui->laskutusCombo->addItem( QIcon(":/pic/pdf.png"), tr("Tallenna pdf-tiedostoon"), PDF);
    ui->laskutusCombo->addItem( QIcon(":/pic/tyhja.png"), tr("Ei tulosteta"), EITULOSTETA);

    int indeksi =   ui->laskutusCombo->findData(nykyinen);
    if( indeksi < 0) {
        qDebug() << "Maksutapa ei käytössä";
        ui->laskutusCombo->setCurrentIndex(0);
    } else {
        ui->laskutusCombo->setCurrentIndex(indeksi);
    }
    paivitetaanLaskutapoja_ = false;
}

void LaskuDialogi::laskutusTapaMuuttui()
{
    int laskutustapa = ui->laskutusCombo->currentData().toInt();
    if( laskutustapa == SAHKOPOSTI)
    {
        ui->valmisNappi->setText( tr("Tallenna ja lähetä sähköpostilla"));
        ui->valmisNappi->setIcon(QIcon(":/pic/email.png"));

    } else if( laskutustapa == PDF) {
        ui->valmisNappi->setText( tr("Tallenna ja toimita"));
        ui->valmisNappi->setIcon(QIcon(":/pic/pdf.png"));
    } else if( laskutustapa == EITULOSTETA) {
        ui->valmisNappi->setText( tr("Tallenna reskontraan"));
        ui->valmisNappi->setIcon(QIcon(":/pic/ok.png"));
    } else if( laskutustapa == POSTITUS){
        ui->valmisNappi->setText( tr("Tallenna ja postita"));
        ui->valmisNappi->setIcon(QIcon(":/pic/mail.png"));
    } else if( laskutustapa == VERKKOLASKU) {
        ui->valmisNappi->setText(tr("Tallenna ja lähetä"));
        ui->valmisNappi->setIcon(QIcon(":/pic/verkkolasku.png"));
    } else {
        ui->valmisNappi->setText( tr("Tallenna ja tulosta"));
        ui->valmisNappi->setIcon(QIcon(":/pic/tulosta.png"));
    }

    if( laskutustapa == VERKKOLASKU && !paivitetaanLaskutapoja_) {
        KpKysely *kysely = kpk( QString("/kumppanit/%1").arg( ui->asiakas->id()));
        connect( kysely, &KpKysely::vastaus, this, &LaskuDialogi::asiakasHaettuLadattaessa);
        kysely->kysy();
    }


    ui->osoiteEdit->setEnabled( laskutustapa != VERKKOLASKU);
    ui->email->setVisible( laskutustapa != VERKKOLASKU );
    ui->emailLabel->setVisible( laskutustapa != VERKKOLASKU);

}

void LaskuDialogi::maksuTapaMuuttui()
{
    int maksutapa = ui->maksuCombo->currentData().toInt();

    ui->eraLabel->setVisible( maksutapa != KATEINEN );
    ui->eraDate->setVisible( maksutapa != KATEINEN );
    ui->viivkorkoLabel->setVisible( maksutapa != KATEINEN );
    ui->viivkorkoSpin->setVisible( maksutapa != KATEINEN );

    ui->hyvitaEnnakkoNappi->setVisible( maksutapa != ENNAKKOLASKU
                                        && tyyppi() == TositeTyyppi::MYYNTILASKU &&
                                        ennakkoModel_->rowCount());

    rivit_->asetaEnnakkolasku(this->ui->maksuCombo->currentData().toInt() == ENNAKKOLASKU);

    ui->toimituspvmLabel->setVisible(maksutapa != ENNAKKOLASKU);
    ui->toimitusDate->setVisible(maksutapa != ENNAKKOLASKU);
    ui->jaksoViivaLabel->setVisible(maksutapa != ENNAKKOLASKU);
    ui->jaksoDate->setVisible( maksutapa != ENNAKKOLASKU);

}

QVariantMap LaskuDialogi::data(QString otsikko) const
{
    QVariantMap map;

    // Laskun kirjauspäivämäärä on laskuperusteisella ja käteisellä laskun päivämäärä,
    // suoriteperusteisella toimituspäivämäärä

    QDate pvm = kp()->paivamaara();
    if( ui->maksuCombo->currentData().toInt() == SUORITEPERUSTE )
        pvm = ui->toimitusDate->date();

    if( tositeId_ )
        map.insert("id", tositeId_);
    if( tunniste_)
        map.insert("tunniste", tunniste_);

    QString laskutettava;

    if( ui->asiakas->id()) {
        QVariantMap kmap;
        kmap.insert("id", ui->asiakas->id());
        map.insert("kumppani", kmap);
        laskutettava = ui->asiakas->nimi();
    } else {
        laskutettava = ui->osoiteEdit->toPlainText().split('\n').value(0);
    }

    if(otsikko.isEmpty())
        otsikko = ui->otsikkoEdit->text();
    if( otsikko.isEmpty())
        otsikko = laskutettava;


    map.insert("otsikko", otsikko);
    map.insert("pvm", pvm);     // Laskupäivä vai toimituspäivä ???
    map.insert("tyyppi",  tyyppi_);
    map.insert("rivit", rivit_->rivit());

    QVariantMap lasku;

    if( !ui->lisatietoEdit->toPlainText().isEmpty()) {
        map.insert("info", ui->lisatietoEdit->toPlainText());
        lasku.insert("lisatiedot", ui->lisatietoEdit->toPlainText());
    }

    if( laskunnumero_) {
        lasku.insert("numero", laskunnumero_);        
    }
    if( !viite_.isEmpty()) {
        lasku.insert("viite", viite_);
        map.insert("viite", viite_);
    }

    if( !asAlvTunnus_.isEmpty())
        lasku.insert("alvtunnus", asAlvTunnus_);

    if( !ui->email->text().isEmpty())
        lasku.insert("email", ui->email->text());
    if( !ui->osoiteEdit->toPlainText().isEmpty())
        lasku.insert("osoite", ui->osoiteEdit->toPlainText());
    if( !ui->asViiteEdit->text().isEmpty())
        lasku.insert("asviite", ui->asViiteEdit->text());

    lasku.insert("pvm", kp()->paivamaara());
    map.insert("laskupvm", kp()->paivamaara());

    lasku.insert("kieli", ui->kieliCombo->currentData());
    lasku.insert("viivkorko", ui->viivkorkoSpin->value());
    lasku.insert("laskutapa", ui->laskutusCombo->currentData().toInt());
    if( ui->maksuCombo->currentData().toInt() != ENNAKKOLASKU)  // Ennakkolaskulla ei ole toimituspäivää
        lasku.insert("toimituspvm", ui->toimitusDate->date());
    if( ui->jaksoDate->date().isValid())
        lasku.insert("jaksopvm", ui->jaksoDate->date());
    if( (rivit_->yhteensa() > 1e-3 || tyyppi() == TositeTyyppi::MAKSUMUISTUTUS ) &&
            ui->maksuCombo->currentData().toInt() != KATEINEN) {
        lasku.insert("erapvm", ui->eraDate->date());
        map.insert("erapvm", ui->eraDate->date());
    }
    lasku.insert("maksutapa", ui->maksuCombo->currentData());
    lasku.insert("otsikko", ui->otsikkoEdit->text());
    lasku.insert("saate", ui->saateEdit->toPlainText());
    if( tyyppi_ != TositeTyyppi::MYYNTILASKU) {
        lasku.insert("alkupNro", alkupLasku_);
        lasku.insert("alkupPvm", alkupPvm_);
    }

    if( !aiemmat_.isEmpty()) {
        lasku.insert("aiemmat", aiemmat_);
        lasku.insert("aiempisaldo", aiempiSaldo_);
    }
    lasku.insert("summa", rivit_->yhteensa());

    if( tyyppi() == TositeTyyppi::MAKSUMUISTUTUS )
        lasku.insert("tyyppi","MAKSUMUISTUTUS");
    else if( tyyppi() == TositeTyyppi::HYVITYSLASKU)
        lasku.insert("tyyppi", "HYVITYSLASKU");
    else if( ui->maksuCombo->currentData().toInt() == ENNAKKOLASKU)
        lasku.insert("tyyppi","ENNAKKOLASKU");
    else if( ui->maksuCombo->currentData().toInt() == KATEINEN)
        lasku.insert("tyyppi", "KUITTI");
    else
        lasku.insert("tyyppi","LASKU");


    map.insert("lasku", lasku);

    if( tyyppi() == TositeTyyppi::MAKSUMUISTUTUS) {
        taydennaMaksumuistutuksenData(map);
    } else {

    // Sitten pitäisi arpoa viennit
        QVariantList viennit;
        // Laskulla on AINA vastakirjaus, jotta tulee laskuluetteloon ;)
        QVariantMap vasta = vastakirjaus(pvm, otsikko);        
        viennit.append( vasta );
        viennit.append( rivit_->viennit( pvm, ui->toimitusDate->date(), ui->jaksoDate->date(),
                                         otsikko, ui->maksuCombo->currentData().toInt() == ENNAKKOLASKU,
                                         ui->maksuCombo->currentData().toInt() == KATEINEN,
                                         ui->asiakas->id()) );

        map.insert("viennit", viennit);
    }

    if( kp()->asetukset()->onko("erisarjaan")  )
        map.insert("sarja", kp()->tositeTyypit()->sarja(tyyppi(), ui->maksuCombo->currentData().toInt() == KATEINEN) );

    return map;
}



void LaskuDialogi::alustaRiviTab()
{

    QSortFilterProxyModel *proxy = new QSortFilterProxyModel(this);
    proxy->setSourceModel( kp()->tuotteet() );

    ui->tuoteView->setModel(proxy);
    proxy->setSortLocaleAware(true);
    proxy->setFilterCaseSensitivity(Qt::CaseInsensitive);
    ui->tuoteView->sortByColumn(TuoteModel::NIMIKE, Qt::AscendingOrder);
    ui->tuoteView->horizontalHeader()->setSectionResizeMode(TuoteModel::NIMIKE, QHeaderView::Stretch);
    ui->tuoteView->setContextMenuPolicy(Qt::CustomContextMenu);
    connect( ui->tuoteView, &QTableView::customContextMenuRequested, this, &LaskuDialogi::tuotteidenKonteksiValikko);


    ui->rivitView->setModel(rivit_);

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

    connect( ui->uusituoteNappi, &QPushButton::clicked, this, &LaskuDialogi::lisaaTuote);
    connect( ui->lisaaRiviNappi, &QPushButton::clicked, [this] { this->rivit_->lisaaRivi();} );
    connect( ui->poistaRiviNappi, &QPushButton::clicked, [this] {
        if( this->ui->rivitView->currentIndex().isValid())
                this->rivit_->poistaRivi( ui->rivitView->currentIndex().row());
    });

    ui->splitter->setStretchFactor(0,1);
    ui->splitter->setStretchFactor(1,3);

    connect( ui->tuoteView, &QTableView::clicked, [this] (const QModelIndex& index)
        { this->rivit_->lisaaRivi( index.data(TuoteModel::TuoteMapRooli).toMap() ); }  );

}

QVariantMap LaskuDialogi::vastakirjaus(const QDate& pvm, const QString &otsikko) const
{
    TositeVienti vienti;

    vienti.setPvm( pvm );
    int maksutapa = ui->maksuCombo->currentData().toInt();
    if( maksutapa == KATEINEN)
        vienti.setTili( kp()->asetukset()->luku("LaskuKateistili"));
    else if( maksutapa == ENNAKKOLASKU)
        vienti.setTili( kp()->asetukset()->luku("LaskuEnnakkosaatavat",1715) );
    else {
        vienti.setTili( kp()->asetukset()->luku("LaskuSaatavatili") );        
    }
    if(  maksutapa != KATEINEN)
        vienti.setEra( -1 );


    if( ui->asiakas->id())
        vienti.setKumppani( ui->asiakas->id());

    double summa = rivit_->yhteensa();
    if( summa > 0) {
        vienti.setDebet(summa);
    } else
        vienti.setKredit(0-summa);

    if( era_ && maksutapa != KATEINEN) {    // #821 Käteislasku on heti maksettu
        if( tyyppi() == TositeTyyppi::MYYNTILASKU) {
            vienti.insert("id", era_);
        }
        vienti.setEra(era_);
    }

    vienti.setTyyppi(TositeVienti::MYYNTI + TositeVienti::VASTAKIRJAUS);
    vienti.setSelite( otsikko );

    return std::move(vienti);
}

void LaskuDialogi::alustaMaksutavat()
{
    ui->maksuCombo->addItem(QIcon(":/pic/lasku.png"), tr("Lasku"), LASKU);
    if( tyyppi_ == TositeTyyppi::MYYNTILASKU) {
        ui->maksuCombo->addItem(QIcon(":/pic/kateinen.png"), tr("Käteinen"), KATEINEN);
        ui->maksuCombo->addItem(QIcon(":/pic/ennakkolasku.png"), tr("Ennakkolasku"), ENNAKKOLASKU);
        ui->maksuCombo->addItem(QIcon(":/pic/suorite.png"), tr("Suoriteperusteinen lasku"), SUORITEPERUSTE);
    }
}

void LaskuDialogi::ohje()
{
    if( ui->maksuCombo->currentData().toInt() == ENNAKKOLASKU )
        kp()->ohje("laskutus/ennakko");
    else if( tyyppi_ == TositeTyyppi::HYVITYSLASKU )
        kp()->ohje("laskutus/hyvitys");
    else if( tyyppi_ == TositeTyyppi::MAKSUMUISTUTUS )
        kp()->ohje("laskutus/muistutus");
    else if( ryhmalasku_ )
        kp()->ohje("laskutus/ryhmalasku");
    else
        kp()->ohje("laskutus/uusi");
}

qlonglong LaskuDialogi::laskeViivastysKorko() const
{
    if( !ui->mmViivastysCheck->isChecked())
        return 0;

    QDate mista = ui->mmViivastysAlkaa->date();
    QDate mihin = ui->mmViivastysLoppuu->date();
    qlonglong paivat = mista.daysTo(mihin);
    return qRound64(aiempiSaldo_ * ui->viivkorkoSpin->value() * paivat / mihin.daysInYear());
}

void LaskuDialogi::taydennaMaksumuistutuksenData(QVariantMap &map) const
{
    QVariantMap lasku = map.take("lasku").toMap();
    MyyntiLaskunTulostaja tulostaja(lasku.value("kieli").toString());

    QVariantList rivit;
    QVariantList viennit;
    qlonglong kulut = 0;

    if( ui->mmMuistutusCheck->isChecked()) {

        TositeVienti mmvienti;
        mmvienti.setPvm(kp()->paivamaara());                                                 
        mmvienti.setTili(kp()->asetukset()->luku("LaskuMaksumuistutustili",9170)); // Tämä asetuksiin
        mmvienti.setTyyppi(TositeTyyppi::TULO + TositeVienti::KIRJAUS);
        mmvienti.setKredit(ui->mmMuistutusMaara->value());
        kulut+=qRound64(ui->mmMuistutusMaara->value() * 100.0);
        if(ui->asiakas->id())
            mmvienti.setKumppani(ui->asiakas->id());
        viennit.append(mmvienti);

        QVariantMap mmmap;
        mmmap.insert("nimike", tulostaja.t("muistutusmaksu"));   // Tähän käännös
        mmmap.insert("myyntikpl",1);
        mmmap.insert("ahinta", ui->mmMuistutusMaara->value());
        mmmap.insert("tili",kp()->asetukset()->luku("LaskuMaksumuistustili",9170));
        rivit.append(mmmap);

        lasku.insert("muistutusmaksu", ui->mmMuistutusMaara->value());
    }
    if( ui->mmViivastysCheck->isChecked()) {
        QDate korkopaiva = ui->mmViivastysAlkaa->date();
        QDate loppupaiva = ui->mmViivastysLoppuu->date();

        // Nyt voidaan laskea viivästyskorko
        if( ui->viivkorkoSpin->value() > 1e-5 && korkopaiva.isValid()) {
            qlonglong paivat = korkopaiva.daysTo(loppupaiva);
            qlonglong vkorkosnt = qRound64( aiempiSaldo_ * ui->viivkorkoSpin->value() * paivat / loppupaiva.daysInYear() );

            QString selite = tulostaja.t("viivkorko") +
                    QString(" %1 - %2")
                    .arg(korkopaiva.toString("dd.MM.yyyy"))
                    .arg(loppupaiva.toString("dd.MM.yyyy"));

            TositeVienti korkovienti;
            korkovienti.setPvm( kp()->paivamaara());
            korkovienti.setTili(kp()->asetukset()->luku("LaskuViivastyskorkotili",9170));
            korkovienti.setTyyppi(TositeTyyppi::TULO + TositeVienti::KIRJAUS);
            korkovienti.setKredit(vkorkosnt);
            korkovienti.setSelite(selite);
            if(ui->asiakas->id())
                korkovienti.setKumppani(ui->asiakas->id());
            kulut += vkorkosnt;
            viennit.append(korkovienti);

            QVariantMap komap;
            komap.insert("nimike", selite);
            komap.insert("myyntikpl", paivat);
            komap.insert("ahinta", (1.00 * aiempiSaldo_ * ui->viivkorkoSpin->value() / loppupaiva.daysInYear() / 100));
            komap.insert("tili", kp()->asetukset()->luku("LaskuViivastyskorkotili",9170));
            rivit.append(komap);

            lasku.insert("korkoalkaa", korkopaiva);
            lasku.insert("korkoloppuu", loppupaiva);
            lasku.insert("korko", vkorkosnt / 100.0);
        }
    }

    // Lisätään aina vastakirjaus, jotta näkyy laskuluettelossa

    TositeVienti vienti;
    vienti.setEra(era_);
    vienti.setPvm(kp()->paivamaara());
    vienti.setTili( kp()->asetukset()->luku("LaskuSaatavatili") );
    vienti.setTyyppi(TositeTyyppi::TULO + TositeVienti::VASTAKIRJAUS);
    vienti.setSelite(ui->otsikkoEdit->text());
    if(ui->asiakas->id()) {
        vienti.setKumppani(ui->asiakas->id());
    }
    vienti.setDebet(kulut);
    viennit.insert(0, vienti);

    lasku.insert("maksutapa", LASKU);

    map.insert("rivit", rivit);
    map.insert("viennit", viennit);
    map.insert("lasku",lasku);
}

void LaskuDialogi::naytaLoki()
{
    NaytinIkkuna *naytin = new NaytinIkkuna();
    QVariant var = ui->lokiView->currentIndex().data(Qt::UserRole);

    QString data = QString::fromUtf8( QJsonDocument::fromVariant(var).toJson(QJsonDocument::Indented) );
    naytin->nayta(data);
}


void LaskuDialogi::tallenna(Tosite::Tila moodi)
{

    QVariantMap map = data();

    for(QVariant var : map.value("viennit").toList()) {
        TositeVienti vienti = var.toMap();
        if( !vienti.tili() ) {
            QMessageBox::critical(this, tr("Tallennusvirhe"),tr("Tiliöinnit ovat puutteellisia."));
            return;
        } else if( kp()->tilitpaatetty() > vienti.pvm() || kp()->tilikaudet()->kirjanpitoLoppuu() < vienti.pvm()) {
            QMessageBox::critical(this, tr("Tallennusvirhe"), tr("Päivämäärälle %1 ei ole avointa tilikautta")
                                  .arg(vienti.pvm().toString("dd.MM.yyyy")));
            return;
        } else if( kp()->alvIlmoitukset()->onkoIlmoitettu(vienti.pvm()) && vienti.alvKoodi() != AlvKoodi::EIALV ) {
            QMessageBox::critical(this, tr("Tallennusvirhe"), tr("Päivämäärälle %1 on jo annettu arvonlisäveroilmoitus")
                                  .arg(vienti.pvm().toString("dd.MM.yyyy")));
            return;
        } else if( asiakkaanAlvTunnus().startsWith("FI") && (vienti.alvKoodi() == AlvKoodi::YHTEISOMYYNTI_TAVARAT ||
                                                             vienti.alvKoodi()==AlvKoodi::YHTEISOMYYNTI_PALVELUT)) {

            QMessageBox::critical(this, tr("Yhteisömyynti"),
                                  tr("Yhteisömyyntiä ei voi tehdä suomalaiselle asiakkaalle."));
            return;
        } else if( asiakkaanAlvTunnus().isEmpty() && (vienti.alvKoodi() == AlvKoodi::RAKENNUSPALVELU_MYYNTI ||
                                                      vienti.alvKoodi() == AlvKoodi::YHTEISOMYYNTI_TAVARAT ||
                                                      vienti.alvKoodi() == AlvKoodi::YHTEISOMYYNTI_PALVELUT)) {
            QMessageBox::critical(this, tr("Käänteinen arvonlisävero"),
                                  tr("Käytettäessä käänteistä arvonlisäveroa on asiakkaalle "
                                     "määriteltävä alv-tunnus."));
            return;
        }
    }

    map.insert("tila", moodi);

    if( ryhmalasku_ ) {
        ryhmalaskuTab_->model()->tallennaLaskut(map);
    } else {
        KpKysely *kysely;
        if( !tositeId_ )
            kysely = kpk("/tositteet/", KpKysely::POST);
        else
            kysely = kpk( QString("/tositteet/%1").arg(tositeId_), KpKysely::PUT);

        connect( kysely, &KpKysely::vastaus, [this, moodi] (QVariant* data)  { this->tallennusValmis(data, moodi == Tosite::LAHETETAAN); } );
        connect( kysely, &KpKysely::virhe, [this] (int koodi, const QString& selite) { QMessageBox::critical(this, tr("Tallennusvirhe"),
                                                                                                             tr("Laskun tallennus epäonnistui\n%1 %2").arg(koodi).arg(selite)); });
        kysely->kysy( map );
    }
}

void LaskuDialogi::tallennusValmis(QVariant *vastaus, bool toimita)
{
    // Tallennetaan ensin liite
    QVariantMap map = vastaus->toMap();

    QByteArray liite = MyyntiLaskunTulostaja::pdf( map );
    KpKysely *liitetallennus = kpk( QString("/liitteet/%1/lasku").arg(map.value("id").toInt()), KpKysely::PUT);
    QMap<QString,QString> meta;
    meta.insert("Filename", QString("lasku%1.pdf").arg( map.value("lasku").toMap().value("numero").toInt() ) );
    liitetallennus->lahetaTiedosto(liite, meta);

    // Mahdollinen laskun toimittaminen

    QDialog::accept();
    emit kp()->kirjanpitoaMuokattu();

    if( toimita ) {

        MyyntiLaskujenToimittaja *toimittaja = new MyyntiLaskujenToimittaja();
        QList<QVariantMap> lista;
        lista.append(map);
        toimittaja->toimitaLaskut(lista);
    } else if( map.value("tila").toInt()==Tosite::VALMISLASKU)
        emit tallennettuValmiina();

}

void LaskuDialogi::ennakkoHyvitysData(int eraid, double eurot, QVariant *data)
{
    QVariantMap map = data->toMap();
    QVariantMap rivi;

    QVariantMap vienti;
    for(auto item : map.value("viennit").toList()) {
        vienti = item.toMap();
        if(vienti.value("era").toMap().value("id").toInt() == eraid)
            break;
    }

    MyyntiLaskunTulostaja tulostaja(ui->kieliCombo->currentData().toString());
    rivi.insert("tili", vienti.value("tili"));
    rivi.insert("era", eraid);
    rivi.insert("myyntikpl",1);
    rivi.insert("ahinta", 0 - eurot);
    int alvkoodi = vienti.value("alvkoodi").toInt();
    rivi.insert("alvkoodi", alvkoodi == AlvKoodi::ENNAKKOLASKU_MYYNTI ? AlvKoodi::MYYNNIT_NETTO : alvkoodi );
    rivi.insert("alvprosentti", vienti.value("alvprosentti").toDouble());
    rivi.insert("ennakkohyvitys", eraid);
    rivi.insert("nimike", tulostaja.t("enhyri").arg(map.value("lasku").toMap().value("numero").toString()));
    rivit_->lisaaRivi(rivi);
}

void LaskuDialogi::alustaRyhmalasku()
{
    ryhmalaskuTab_ = new RyhmalaskuTab;
    ui->tabWidget->addTab( ryhmalaskuTab_, QIcon(":/pic/asiakkaat.png"), tr("Laskutettavat"));
    ui->asiakasLabel->hide();
    ui->asiakas->hide();
    ui->osoiteLabel->hide();
    ui->osoiteEdit->hide();
    ui->emailLabel->hide();
    ui->email->hide();
    ui->asviiteLabel->hide();
    ui->asViiteEdit->hide();
    ui->kieliCombo->hide();
    ui->valmisNappi->hide();
    ui->laskutusCombo->hide();
    ui->luonnosNappi->setEnabled(false);
    connect( ryhmalaskuTab_->model(),  &LaskutettavatModel::tallennettu, this, &LaskuDialogi::accept );
    connect( ryhmalaskuTab_->model(),  &LaskutettavatModel::tallennettu, this, &LaskuDialogi::tallennettuValmiina );
    connect( ryhmalaskuTab_->model(), &LaskutettavatModel::rowsInserted, this, &LaskuDialogi::paivitaNapit);
    setWindowTitle(tr("Ryhmälasku"));
}

void LaskuDialogi::lataa(const QVariantMap &map)
{

    QVariantMap vienti = map.value("viennit").toList().value(0).toMap();
    tyyppi_ = map.value("tyyppi").toInt();    
    alustaMaksutavat();

    asiakasId_ = map.value("kumppani").toMap().value("id").toInt();
    ui->asiakas->set( asiakasId_,
                      map.value("kumppani").toMap().value("nimi").toString());
    if(map.contains("kumppani")) {
        ennakkoModel_->lataaErat(map.value("kumppani").toMap().value("id").toInt());
        KpKysely *kysely = kpk( QString("/kumppanit/%1").arg(map.value("kumppani").toMap().value("id").toInt()) );
        connect( kysely, &KpKysely::vastaus, this, &LaskuDialogi::asiakasHaettuLadattaessa);
        kysely->kysy();
    }

    QVariantMap lasku = map.value("lasku").toMap();

    ui->osoiteEdit->setPlainText( lasku.value("osoite").toString());
    ui->email->setText( lasku.value("email").toString() );
    ui->asViiteEdit->setText( lasku.value("asviite").toString() );
    ui->kieliCombo->setCurrentIndex( ui->kieliCombo->findData( lasku.value("kieli").toString() ) );

    int laskutapa = lasku.value("laskutapa").toInt();
    if( laskutapa == VERKKOLASKU) { verkkolaskutettava_=true; paivitaLaskutustavat(); }
    ui->laskutusCombo->setCurrentIndex( ui->laskutusCombo->findData( laskutapa ));

    ui->maksuCombo->setCurrentIndex( ui->maksuCombo->findData( lasku.value("maksutapa").toInt() ));
    ui->toimitusDate->setDate( lasku.value("toimituspvm").toDate() );
    ui->jaksoDate->setDate( lasku.value("jaksopvm").toDate());
    ui->eraDate->setDate( lasku.value("erapvm").toDate());
    ui->otsikkoEdit->setText( lasku.value("otsikko").toString());
    ui->lisatietoEdit->setPlainText( map.value("info").toString());
    ui->saateEdit->setPlainText(lasku.value("saate").toString());
    ui->viivkorkoSpin->setValue( lasku.value("viivkorko").toDouble() );

    if( map.value("tila").toInt() > Tosite::LUONNOS)
        ui->luonnosNappi->hide();

    tositeId_ = map.value("id").toInt();
    laskunnumero_ = lasku.value("numero").toLongLong();
    alkupLasku_ = lasku.value("alkupNro").toInt();
    alkupPvm_ = lasku.value("alkupPvm").toDate();
    viite_ = map.value("viite").toString();
    tunniste_ = map.value("tunniste").toInt();
    era_ = vienti.value("era").toMap().value("id").toInt() > 0
            ? vienti.value("era").toMap().value("id").toInt()
            : vienti.value("id").toInt() ;
    asAlvTunnus_ = lasku.value("alvtunnus").toString();
    aiemmat_ = lasku.value("aiemmat").toList();
    aiempiSaldo_ = lasku.value("aiempisaldo").toDouble();

    tallennettu_ = data();
    paivitaSumma();

    if( !viite_.isEmpty()) {
        ui->viiteLabel->show();
        ui->viiteText->show();
        ui->viiteText->setText(viite_);
        ui->infoLabel->show();
        ui->infoLabel->setText( tr("Laskua ei lähetetty"));

        QVariantList loki = map.value("loki").toList();
        for(auto lokirivi : loki) {
            QVariantMap lokimap = lokirivi.toMap();
            if( lokimap.value("tila").toInt() == Tosite::LAHETETTYLASKU)
            {
                ui->infoLabel->setText( tr("Lähetetty %1").arg( lokimap.value("aika").toDateTime().toString("dd.MM.yyyy hh.mm") ));
                break;
            }
        }
        if( vienti.contains("era") && vienti.value("era").toMap().value("id").toInt() &&
                qAbs(vienti.value("era").toMap().value("saldo").toDouble()) < 1e-5) {
            ui->maksettuCheck->show();
            ui->infoLabel->setText( tr("Maksettu "));
            ui->infoLabel->setStyleSheet("color: green;");
            ui->valmisNappi->hide();
        }

    }

    if( tyyppi() == TositeTyyppi::MAKSUMUISTUTUS) {
        double avoin = lasku.value("aiempisaldo").toDouble();
        ui->mmAvoin->setText(QString("%L1 €").arg( avoin,0,'f',2));
        double muikkari = lasku.value("muistutusmaksu").toDouble();
        ui->mmMuistutusCheck->setChecked( muikkari > 1e-5);
        ui->mmMuistutusMaara->setValue( muikkari );
        double korko = lasku.value("korko").toDouble();
        ui->mmViivastysCheck->setChecked( korko > 1e-5);
        ui->mmViivastysAlkaa->setDate( lasku.value("korkoalkaa").toDate() );
        ui->mmViivastysLoppuu->setDate( lasku.value("korkoloppuu").toDate() );
        ui->mmViivastysMaara->setText(QString("%L1 €").arg( korko,0,'f',2));

        qlonglong yht = qRound64( avoin * 100 ) + qRound64( muikkari * 100 ) + qRound64( korko * 100);

        ui->mmYhteensa->setText(QString("%L1 €").arg( yht / 100.0 ,0,'f',2));
    }

    int tila = map.value("tila").toInt();
    ui->luonnosNappi->setVisible( tila == Tosite::LUONNOS );
    ui->tallennaNappi->setVisible( tila < Tosite::KIRJANPIDOSSA );

    maksuTapaMuuttui();
    paivitaNakyvat();
    paivitaNapit();   

    TositeLoki *lokimodel = new TositeLoki(this);
    lokimodel->lataa(map.value("loki").toList());
    ui->lokiView->setModel(lokimodel);
    ui->lokiView->resizeColumnsToContents();
    ui->lokiView->horizontalHeader()->setSectionResizeMode(TositeLoki::KAYTTAJA, QHeaderView::Stretch);
    connect( ui->lokiView, &QTableView::clicked, this, &LaskuDialogi::naytaLoki);
}

void LaskuDialogi::paivitaNakyvat()
{
    setWindowTitle( kp()->tositeTyypit()->nimi(tyyppi_) + " " +
                    (laskunnumero_ > 0 ? QString::number(laskunnumero_)
                                      : ""));

    if( tyyppi() == TositeTyyppi::HYVITYSLASKU ) {
        setWindowTitle( windowTitle() + tr(" laskulle %1").arg(alkupLasku_) );
        ui->toimituspvmLabel->setText( tr("Hyvityspäivä"));
        ui->eraLabel->hide();
        ui->eraDate->hide();
        ui->viivkorkoLabel->hide();
        ui->viivkorkoSpin->hide();
    }
    ui->maksuCombo->setVisible( tyyppi() == TositeTyyppi::MYYNTILASKU );
    int maksutapa = ui->maksuCombo->currentData().toInt();
    ui->jaksoViivaLabel->setVisible( tyyppi() == TositeTyyppi::MYYNTILASKU && maksutapa != ENNAKKOLASKU);
    ui->jaksoDate->setVisible( tyyppi() == TositeTyyppi::MYYNTILASKU && maksutapa != ENNAKKOLASKU);


}


void LaskuDialogi::lisaaEnnakkoHyvitys(int eraId, double eurot)
{
    KpKysely *kysely = kpk("/tositteet");
    kysely->lisaaAttribuutti("vienti", eraId);
    connect( kysely, &KpKysely::vastaus,
             [this, eraId, eurot] (QVariant *data) { ennakkoHyvitysData(eraId, eurot, data); });
    kysely->kysy();
}


