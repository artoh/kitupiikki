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

#include "kirjaus/verodialogi.h"
#include "naytin/naytinikkuna.h"
#include "naytin/naytinview.h"
#include "validator/ytunnusvalidator.h"
#include "asiakkaatmodel.h"

#include "ui_yhteystiedot.h"

#include "validator/ytunnusvalidator.h"

#include "laskurivitmodel.h"
#include "model/tositevienti.h"

#include "myyntilaskuntulostaja.h"
#include "model/tosite.h"

#include "db/tositetyyppimodel.h"

#include "myyntilaskujentoimittaja.h"

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

LaskuDialogi::LaskuDialogi( const QVariantMap& data) :
    rivit_(new LaskuRivitModel(this, data.value("rivit").toList())),
    ui( new Ui::LaskuDialogi)
{
    ui->setupUi(this);

    setAttribute(Qt::WA_DeleteOnClose);
    resize(800,600);
    restoreGeometry( kp()->settings()->value("LaskuDialogi").toByteArray());

    ui->kieliCombo->addItem(QIcon(":/liput/fi.png"), tr("Suomi"), "FI");
    ui->kieliCombo->addItem(QIcon(":/liput/sv.png"), tr("Ruotsi"), "SV");
    ui->kieliCombo->addItem(QIcon(":/liput/en.png"), tr("Englanti"), "EN");

    ui->asiakas->alusta();

    connect( ui->esikatseluNappi, SIGNAL(clicked(bool)), this, SLOT(esikatselu()));

    connect( rivit_, &LaskuRivitModel::dataChanged, this, &LaskuDialogi::paivitaSumma);
    connect( rivit_, &LaskuRivitModel::rowsInserted, this, &LaskuDialogi::paivitaSumma);
    connect( rivit_, &LaskuRivitModel::modelReset, this, &LaskuDialogi::paivitaSumma);

    lisaaRiviTab();
    connect( ui->asiakas, &AsiakasToimittajaValinta::valittu, this, &LaskuDialogi::asiakasValittu);
    connect( ui->email, &QLineEdit::textChanged, this, &LaskuDialogi::paivitaLaskutustavat);
    connect( ui->laskutusCombo, &QComboBox::currentTextChanged, this, &LaskuDialogi::laskutusTapaMuuttui);

    connect( ui->luonnosNappi, &QPushButton::clicked, [this] () { this->tallenna(Tosite::LUONNOS); });
    connect( ui->tallennaNappi, &QPushButton::clicked, [this] () { this->tallenna(Tosite::VALMISLASKU);});
    connect( ui->valmisNappi, &QPushButton::clicked, [this] () { this->tallenna(Tosite::KIRJANPIDOSSA);});

    paivitaLaskutustavat();
    ui->jaksoDate->setNull();

    ui->viiteLabel->hide();
    ui->viiteText->hide();
    ui->maksettuCheck->hide();
    ui->infoLabel->hide();

    if( !data.isEmpty())
        lataa(data);
    else {
        QDate erapvm = kp()->paivamaara().addDays( kp()->asetukset()->luku("LaskuMaksuaika") );
        while( erapvm.dayOfWeek() > 5)
            erapvm = erapvm.addDays(1);
        ui->eraDate->setDate( erapvm );
        alustaMaksutavat();
        ui->saateEdit->setPlainText( kp()->asetus("EmailSaate") );
    }

}


LaskuDialogi::~LaskuDialogi()
{
    kp()->settings()->setValue("LaskuDialogi", saveGeometry());
    delete ui;
}

void LaskuDialogi::paivitaSumma()
{
    ui->summaLabel->setText( QString("%L1 €").arg( rivit_->yhteensa() ,0,'f',2) );
    paivitaNapit();
}

void LaskuDialogi::esikatselu()
{
    esikatsele();
}

void LaskuDialogi::paivitaNapit()
{
    bool tallennettavaa = !rivit_->onkoTyhja();

    ui->luonnosNappi->setEnabled( tallennettavaa );
    ui->tallennaNappi->setEnabled( tallennettavaa );
    ui->valmisNappi->setEnabled( tallennettavaa );
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

void LaskuDialogi::perusteVaihtuu()
{
/*    int peruste = ui->perusteCombo->currentData().toInt();

//    ui->rahaTiliEdit->setVisible( peruste != LaskuModel::MAKSUPERUSTE );
//    ui->rahatiliLabel->setVisible( peruste != LaskuModel::MAKSUPERUSTE );

    ui->viivkorkoLabel->setVisible( peruste != LaskuModel::KATEISLASKU);
    ui->viivkorkoSpin->setVisible( peruste != LaskuModel::KATEISLASKU);
    ui->eraLabel->setVisible( peruste != LaskuModel::KATEISLASKU);
    ui->eraDate->setVisible( peruste != LaskuModel::KATEISLASKU);

    if( peruste == LaskuModel::MAKSUPERUSTE || peruste == LaskuModel::LASKUTUSPERUSTE)
        ui->toimitusDate->setMinimumDate( kp()->tilitpaatetty().addYears(-1));
    else
        // Jos kirjataan toimituspäivälle, pitää toimituspäivän olla tilikaudella
        ui->toimitusDate->setMinimumDate( kp()->tilitpaatetty().addDays(1));


    if( peruste == LaskuModel::SUORITEPERUSTE || peruste == LaskuModel::LASKUTUSPERUSTE )
    {
        ui->rahaTiliEdit->suodataTyypilla("AS");
        ui->rahaTiliEdit->valitseTiliNumerolla( kp()->asetus("LaskuSaatavatili").toInt());
    }
    else if( peruste == LaskuModel::KATEISLASKU)
    {
        ui->rahaTiliEdit->suodataTyypilla("AR");
        ui->rahaTiliEdit->valitseTiliNumerolla( kp()->asetus("LaskuKateistili").toInt());
    }
    model->asetaKirjausperuste(peruste);
*/
}





void LaskuDialogi::rivienKontekstiValikko(QPoint pos)
{
/*    kontekstiIndeksi=ui->rivitView->indexAt(pos);

    QMenu *menu=new QMenu(this);
    if( kontekstiIndeksi.data(LaskuModel::TuoteKoodiRooli).toInt() )
        menu->addAction(QIcon(":/pic/kitupiikki32.png"), tr("Päivitä tuoteluetteloon"), this, SLOT(paivitaTuoteluetteloon()) );
    else
        menu->addAction(QIcon(":/pic/lisaa.png"), tr("Lisää tuoteluetteloon"), this, SLOT(lisaaTuoteluetteloon()));
    menu->popup(ui->rivitView->viewport()->mapToGlobal(pos));*/
}


void LaskuDialogi::poistaLaskuRivi()
{
/*    int indeksi = ui->rivitView->currentIndex().row();
    if( indeksi > -1)
        model->poistaRivi(indeksi);*/
}

void LaskuDialogi::tuotteidenKonteksiValikko(QPoint pos)
{
/*    kontekstiIndeksi = tuoteProxy->mapToSource( ui->tuotelistaView->indexAt(pos) );

    QMenu *menu = new QMenu(this);
    menu->addAction(QIcon(":/pic/poistarivi.png"), tr("Poista tuoteluettelosta"), this, SLOT(poistaTuote()));
    menu->popup( ui->tuotelistaView->viewport()->mapToGlobal(pos)); */
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

void LaskuDialogi::ryhmaNapit(const QItemSelection &valinta)
{
    ui->esikatseluNappi->setEnabled( valinta.size());

}

void LaskuDialogi::lisaaAsiakasListalta(const QModelIndex &indeksi)
{
    QSqlQuery kysely;
    QString nimistr = indeksi.data(AsiakkaatModel::NimiRooli).toString();
    nimistr.remove(QRegExp("['\"]"));

    kysely.exec( QString("SELECT json FROM vienti WHERE asiakas='%1' AND iban is null ORDER BY muokattu DESC").arg( nimistr)) ;
    QString osoite = nimistr;
    QString email;
    QString ytunnus;
    QString verkkolaskuosoite;
    QString verkkolaskuvalittaja;

    if( kysely.next() )
    {
        JsonKentta json;
        json.fromJson( kysely.value(0).toByteArray() );
        email =  json.str("Email");
        ytunnus = json.str("YTunnus");
        verkkolaskuosoite = json.str("VerkkolaskuOsoite");
        verkkolaskuvalittaja = json.str("VerkkolaskuValittaja");

        if( !json.str("Osoite").isEmpty())
        {
            // Haetaan aiempi osoite
            osoite = json.str("Osoite");
        }
    }

}

void LaskuDialogi::lisaaAsiakas()
{
    QDialog yhteystiedot;
    Ui::Yhteystiedot dui;
    dui.setupUi(&yhteystiedot);
    yhteystiedot.setWindowTitle(tr("Lisää laskun saaja"));
    dui.YtunnusEdit->setValidator(new YTunnusValidator(true));
    connect(dui.tallennaNappi, &QPushButton::clicked, &yhteystiedot, &QDialog::accept);
    connect(dui.peruNappi, &QPushButton::clicked, &yhteystiedot, &QDialog::reject);
    connect(dui.nimiEdit, &QLineEdit::textChanged, [dui](const QString& teksti) { dui.osoiteEdit->setPlainText(teksti + "\n"); });
    if( yhteystiedot.exec() == QDialog::Accepted)
    {
        if( !dui.YtunnusEdit->hasAcceptableInput() )
            dui.YtunnusEdit->clear();

    }
}

void LaskuDialogi::tuoAsiakkaitaTiedostosta()
{
}

void LaskuDialogi::poistaValitutAsiakkaat()
{
/*    QList<int> indeksit;
    for(const QModelIndex &indeksi : ui->ryhmaView->selectionModel()->selectedRows())
        indeksit.append( ryhmaProxy_->mapToSource(indeksi).row() );

    // Poistetaan alimmasta alkaen, jotta poistaminen ei sotke
    // rivien numerointia

    qSort( indeksit );
    while( !indeksit.isEmpty())
    {
        model->ryhmaModel()->poista( indeksit.last() );
        indeksit.removeLast();
    }*/
}

void LaskuDialogi::paivitaRyhmanTallennusNappi()
{

}

void LaskuDialogi::verkkolaskuKayttoon()
{
//    ui->verkkolaskuNappi->setEnabled( !ui->verkkoOsoiteEdit->text().isEmpty() && !ui->verkkoValittajaEdit->text().isEmpty() );
}

void LaskuDialogi::asiakasValittu(int asiakasId)
{
    KpKysely *kysely = kpk( QString("/kumppanit/%1").arg(asiakasId) );
    connect( kysely, &KpKysely::vastaus, this, &LaskuDialogi::taytaAsiakasTiedot);
    kysely->kysy();
}

void LaskuDialogi::taytaAsiakasTiedot(QVariant *data)
{
    QVariantMap map = data->toMap();
    ui->osoiteEdit->setPlainText( map.value("nimi").toString() + "\n" +
                                  map.value("osoite").toString() + "\n" +
                                  map.value("postinumero").toString() + " " + map.value("kaupunki").toString());
    ui->email->setText( map.value("email").toString());
    ui->kieliCombo->setCurrentIndex(ui->kieliCombo->findData(map.value("kieli").toString()));
    ui->laskutusCombo->setCurrentIndex(ui->laskutusCombo->findData(map.value("laskutapa")));

    asAlvTunnus_ = map.value("alvtunnus").toString();


    // TODO: Mahdolliset toimitustavat
}

void LaskuDialogi::paivitaLaskutustavat()
{
    int nykyinen = ui->laskutusCombo->currentData().toInt();
    ui->laskutusCombo->clear();

    ui->laskutusCombo->addItem( QIcon(":/pic/tulosta.png"), tr("Tulosta lasku"), TULOSTETTAVA);

    QRegularExpression emailRe(R"(^([\w-]*(\.[\w-]+)?)+@(\w+\.\w+)(\.\w+)*$)");
    if( emailRe.match( ui->email->text()).hasMatch() )
            ui->laskutusCombo->addItem(QIcon(":/pic/email.png"), tr("Lähetä sähköpostilla"), SAHKOPOSTI);
    ui->laskutusCombo->addItem( QIcon(":/pic/pdf.png"), tr("Tallenna pdf-tiedostoon"), PDF);
    ui->laskutusCombo->addItem( QIcon(":/pic/tyhja.png"), tr("Ei tulosteta"), EITULOSTETA);

    ui->laskutusCombo->setCurrentIndex(  ui->laskutusCombo->findData(nykyinen) );
    if( ui->laskutusCombo->currentIndex() < 0)
        ui->laskutusCombo->setCurrentIndex(0);
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
    } else {
        ui->valmisNappi->setText( tr("Tallenna ja tulosta"));
        ui->valmisNappi->setIcon(QIcon(":/pic/tulosta.png"));
    }

}

void LaskuDialogi::maksuTapaMuuttui()
{
    int maksutapa = ui->maksuCombo->currentData().toInt();

    ui->eraLabel->setVisible( maksutapa != KATEINEN );
    ui->eraDate->setVisible( maksutapa != KATEINEN );
    ui->viivkorkoLabel->setVisible( maksutapa != KATEINEN );
    ui->viivkorkoSpin->setVisible( maksutapa != KATEINEN );

}

QVariantMap LaskuDialogi::data() const
{
    QVariantMap map;

    if( tositeId_ )
        map.insert("id", tositeId_);
    if( tunniste_)
        map.insert("tunniste", tunniste_);

    QString laskutettava;
    QString otsikko;

    if( ui->asiakas->id()) {
        map.insert("kumppani", ui->asiakas->id());
        laskutettava = ui->asiakas->nimi();
    } else {
        laskutettava = ui->osoiteEdit->toPlainText().split('\n').value(0);
    }

    if( ui->otsikkoEdit->text().isEmpty())
        otsikko = laskutettava;
    else
        otsikko = ui->otsikkoEdit->text();


    map.insert("otsikko", otsikko);
    map.insert("pvm", kp()->paivamaara() );
    map.insert("tyyppi",  tyyppi_);
    map.insert("rivit", rivit_->rivit());

    if( !ui->lisatietoEdit->toPlainText().isEmpty())
        map.insert("info", ui->lisatietoEdit->toPlainText());

    QVariantMap lasku;

    if( laskunnumero_) {
        lasku.insert("numero", laskunnumero_);        
    }
    if( !viite_.isEmpty()) {
        lasku.insert("viite", viite_);
    }

    if( !asAlvTunnus_.isEmpty())
        lasku.insert("alvtunnus", asAlvTunnus_);

    if( !ui->email->text().isEmpty())
        lasku.insert("email", ui->email->text());
    if( !ui->osoiteEdit->toPlainText().isEmpty())
        lasku.insert("osoite", ui->osoiteEdit->toPlainText());
    if( !ui->asViiteEdit->text().isEmpty())
        lasku.insert("asviite", ui->asViiteEdit->text());

    lasku.insert("kieli", ui->kieliCombo->currentData());
    lasku.insert("viivkorko", ui->viivkorkoSpin->value());
    lasku.insert("laskutapa", ui->laskutusCombo->currentData());
    lasku.insert("toimituspvm", ui->toimitusDate->date());
    if( rivit_->yhteensa() > 1e-3)
        lasku.insert("erapvm", ui->eraDate->date());
    lasku.insert("maksutapa", ui->maksuCombo->currentData());
    lasku.insert("otsikko", ui->otsikkoEdit->text());
    lasku.insert("saate", ui->saateEdit->toPlainText());
    if( tyyppi_ != TositeTyyppi::MYYNTILASKU) {
        lasku.insert("alkupNro", alkupLasku_);
        lasku.insert("alkupPvm", alkupPvm_);
    }

    map.insert("lasku", lasku);


    // Sitten pitäisi arpoa viennit
    QVariantList viennit;
    viennit.append( vastakirjaus( otsikko ) );
    viennit.append( rivit_->viennit( kp()->paivamaara(), ui->toimitusDate->date(), ui->jaksoDate->date(), otsikko ) );

    map.insert("viennit", viennit);


    return map;
}



void LaskuDialogi::lisaaRiviTab()
{
    QSplitter* split = new QSplitter(Qt::Horizontal,this);


    TuoteModel* tuoteModel = new TuoteModel(this);
    tuoteModel->lataa();

    QLineEdit* tuoteFiltterinEditori = new QLineEdit();
    tuoteFiltterinEditori->setPlaceholderText("Etsi tuotetta nimellä");

    QVBoxLayout *leiska = new QVBoxLayout;
    leiska->addWidget(tuoteFiltterinEditori);

    QSortFilterProxyModel *proxy = new QSortFilterProxyModel(this);
    proxy->setSourceModel(tuoteModel);

    QTableView* tuoteView = new QTableView();
    leiska->addWidget(tuoteView);
    tuoteView->setModel(proxy);
    tuoteView->setSelectionBehavior(QTableView::SelectRows);
    tuoteView->setSelectionMode(QTableView::SingleSelection);

    proxy->setSortLocaleAware(true);
    tuoteView->sortByColumn(TuoteModel::NIMIKE, Qt::AscendingOrder);
    tuoteView->horizontalHeader()->setSectionResizeMode(TuoteModel::NIMIKE, QHeaderView::Stretch);

    QWidget *tuoteWg = new QWidget();
    tuoteWg->setLayout(leiska);
    split->addWidget(tuoteWg);


    QTableView* rivitView = new QTableView(this);
    split->addWidget(rivitView);

    rivitView->setModel(rivit_);

    rivitView->horizontalHeader()->setSectionResizeMode(LaskuRivitModel::NIMIKE, QHeaderView::Stretch);
    rivitView->setItemDelegateForColumn(LaskuRivitModel::AHINTA, new EuroDelegaatti());
    rivitView->setItemDelegateForColumn(LaskuRivitModel::TILI, new TiliDelegaatti());
    rivitView->setSelectionMode(QTableView::SingleSelection);


    KohdennusDelegaatti *kohdennusDelegaatti = new KohdennusDelegaatti();
    rivitView->setItemDelegateForColumn(LaskuRivitModel::KOHDENNUS, kohdennusDelegaatti );

    connect( ui->toimitusDate , SIGNAL(dateChanged(QDate)), kohdennusDelegaatti, SLOT(asetaKohdennusPaiva(QDate)));
    connect( tuoteFiltterinEditori, &QLineEdit::textChanged, proxy, &QSortFilterProxyModel::setFilterFixedString);


    rivitView->setItemDelegateForColumn(LaskuRivitModel::BRUTTOSUMMA, new EuroDelegaatti());
    rivitView->setItemDelegateForColumn(LaskuRivitModel::ALV, new LaskutusVeroDelegaatti());

    rivitView->setColumnHidden( LaskuRivitModel::ALV, !kp()->asetukset()->onko("AlvVelvollinen") );
    rivitView->setColumnHidden( LaskuRivitModel::KOHDENNUS, !kp()->kohdennukset()->kohdennuksia());

    split->setStretchFactor(0,1);
    split->setStretchFactor(1,3);

    ui->tabWidget->insertTab(0, split, QIcon(":/pic/vientilista.png"),"Rivit");
    ui->tabWidget->setCurrentWidget(split);

    connect( tuoteView, &QTableView::clicked, [this, proxy, tuoteModel] (const QModelIndex& index)
        { this->rivit_->lisaaRivi( tuoteModel->tuoteMap( proxy->mapToSource(index).row()) ); }  );

}

QVariantMap LaskuDialogi::vastakirjaus(const QString &otsikko) const
{
    TositeVienti vienti;

    vienti.setPvm( kp()->paivamaara() );
    if( ui->maksuCombo->currentData().toInt() == KATEINEN)
        // TODO: Tilinvalinnat
        vienti.setTili( kp()->tilit()->tiliTyypilla(TiliLaji::KATEINEN).numero());
    else {
        vienti.setTili( kp()->tilit()->tiliTyypilla(TiliLaji::MYYNTISAATAVA).numero() );
        if( tallennusTila_ >= Tosite::VALMISLASKU)
            vienti.setEra( -1 );
    }


    if( ui->asiakas->id())
        vienti.insert("kumppani", ui->asiakas->id());

    double summa = rivit_->yhteensa();
    if( summa > 0) {
        vienti.setDebet(summa);
        if( ui->maksuCombo->currentData().toInt() != KATEINEN) {
            vienti.setErapaiva( ui->eraDate->date() );
        }
    } else
        vienti.setKredit(0-summa);

    if( !viite_.isEmpty())
        vienti.setViite( viite_ );

    if( era_ ) {
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
    if( tyyppi_ == TositeTyyppi::MYYNTILASKU)
        ui->maksuCombo->addItem(QIcon(":/pic/kateinen.png"), tr("Käteinen"), KATEINEN);
}

void LaskuDialogi::tallenna(Tosite::Tila moodi)
{
    tallennusTila_ = moodi;

    QVariantMap map = data();
    if( moodi == Tosite::LUONNOS )
        map.insert("tila", Tosite::LUONNOS);
    else if( map.value("tila").toInt() < Tosite::KIRJANPIDOSSA )
        map.insert("tila", Tosite::VALMISLASKU);
    else
        map.insert("tila", Tosite::KIRJANPIDOSSA);


    KpKysely *kysely;
    if( !tositeId_ )
        kysely = kpk("/tositteet/", KpKysely::POST);
    else
        kysely = kpk( QString("/tositteet/%1").arg(tositeId_), KpKysely::PUT);

    connect( kysely, &KpKysely::vastaus, this, &LaskuDialogi::tallennusValmis);
    kysely->kysy( map );
}

void LaskuDialogi::tallennusValmis(QVariant *vastaus)
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

    if( tallennusTila_ == Tosite::KIRJANPIDOSSA) {

        MyyntiLaskujenToimittaja *toimittaja = new MyyntiLaskujenToimittaja();
        QList<QVariantMap> lista;
        lista.append(vastaus->toMap());
        toimittaja->toimitaLaskut(lista);
    }

}

void LaskuDialogi::lataa(const QVariantMap &map)
{

    QVariantMap vienti = map.value("viennit").toList().value(0).toMap();
    tyyppi_ = map.value("tyyppi").toInt();    
    alustaMaksutavat();

    ui->asiakas->set( map.value("kumppani").toMap().value("id").toInt(),
                      map.value("kumppani").toMap().value("nimi").toString());

    QVariantMap lasku = map.value("lasku").toMap();

    ui->osoiteEdit->setPlainText( lasku.value("osoite").toString());
    ui->email->setText( lasku.value("email").toString() );
    ui->asViiteEdit->setText( lasku.value("asviite").toString() );
    ui->kieliCombo->setCurrentIndex( ui->kieliCombo->findData( lasku.value("kieli").toString() ) );
    ui->laskutusCombo->setCurrentIndex( ui->laskutusCombo->findData( lasku.value("laskutapa").toInt() ));
    ui->maksuCombo->setCurrentIndex( ui->maksuCombo->findData( lasku.value("maksutapa").toInt() ));
    ui->toimitusDate->setDate( lasku.value("toimituspvm").toDate() );
    ui->eraDate->setDate( lasku.value("erapvm").toDate());
    ui->otsikkoEdit->setText( lasku.value("otsikko").toString());
    ui->lisatietoEdit->setPlainText( map.value("info").toString());
    ui->saateEdit->setPlainText(lasku.value("saate").toString());

    if( map.value("tila").toInt() > Tosite::LUONNOS)
        ui->luonnosNappi->hide();

    tositeId_ = map.value("id").toInt();
    laskunnumero_ = lasku.value("numero").toLongLong();
    alkupLasku_ = lasku.value("alkupNro").toInt();
    alkupPvm_ = lasku.value("alkupPvm").toDate();
    viite_ = lasku.value("viite").toString();
    tunniste_ = map.value("tunniste").toInt();
    era_ = tyyppi_ == TositeTyyppi::MYYNTILASKU
            ? vienti.value("id").toInt()
            : lasku.value("era").toInt();
    asAlvTunnus_ = lasku.value("alvtunnus").toString();

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
    int tila = map.value("tila").toInt();
    ui->luonnosNappi->setVisible( tila == Tosite::LUONNOS );
    ui->tallennaNappi->setVisible( tila < Tosite::KIRJANPIDOSSA );

    paivitaNakyvat();
    paivitaNapit();

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
    ui->jaksoViivaLabel->setVisible( tyyppi() == TositeTyyppi::MYYNTILASKU);
    ui->jaksoDate->setVisible( tyyppi() == TositeTyyppi::MYYNTILASKU );


}

int LaskuDialogi::laskuIkkunoita()
{
    return 0;
}


