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

#include <QDebug>
#include <QSqlQuery>
#include <QSqlError>
#include <QSqlQueryModel>
#include <QMessageBox>
#include <QIntValidator>
#include <QFileDialog>
#include <QDesktopServices>
#include <QUrl>
#include <QMenu>
#include <QAction>
#include <QPrintDialog>
#include <QPrinter>
#include <QPainter>
#include <QJsonDocument>

#include <QShortcut>
#include <QSettings>

#include <QSortFilterProxyModel>
#include <QCompleter>

#include <QClipboard>
#include <QMimeData>

#include "kirjauswg.h"
#include "liite/liitecache.h"
#include "liite/liitteetmodel.h"

#include "tilidelegaatti.h"
#include "eurodelegaatti.h"
#include "pvmdelegaatti.h"
#include "kohdennusdelegaatti.h"

#include "siirrydlg.h"

#include "db/kirjanpito.h"
#include "naytin/naytinikkuna.h"
#include "ui_kopioitosite.h"

#include "db/tositetyyppimodel.h"

#include "apuri/tulomenoapuri.h"
#include "apuri/siirtoapuri.h"
#include "apuri/tiliote/tilioteapuri.h"
#include "apuri/palkkaapuri.h"
#include "model/tosite.h"
#include "model/tositeviennit.h"
#include "model/tositeloki.h"
#include "tallennettuwidget.h"

#include "selaus/selauswg.h"
#include "arkistoija/arkistoija.h"

#include "db/yhteysmodel.h"
#include "../kierto/kiertowidget.h"
#include "../kierto/kiertomodel.h"
#include "kommentitwidget.h"

#include "muumuokkausdlg.h"
#include "pilvi/pilvimodel.h"
#include "alv/alvilmoitustenmodel.h"

KirjausWg::KirjausWg( QWidget *parent, SelausWg* selaus)
    : QWidget(parent),
      tosite_( new Tosite(this)),
      apuri_(nullptr),              
      selaus_(selaus),
      edellinenSeuraava_( qMakePair(0,0))
{
    ui = new Ui::KirjausWg();
    ui->setupUi(this);

    viennitTab_ = ui->tabWidget->widget(VIENNIT);
    memoTab_ = ui->tabWidget->widget(MUISTIINPANOT);
    liitteetTab_ = ui->tabWidget->widget(LIITTEET);
    varastoTab_ = ui->tabWidget->widget(VARASTO);
    lokiTab_ = ui->tabWidget->widget(LOKI);
    kiertoTab_ = new KiertoWidget(tosite(), this);
    kommentitTab_ = new KommentitWidget(tosite(), this);


    // Tämä pitää säilyttää, jotta saadaan päivämäärä paikalleen
    ui->viennitView->setItemDelegateForColumn( TositeViennit::PVM, new PvmDelegaatti(ui->tositePvmEdit, this));

    connect( ui->lisaaRiviNappi, SIGNAL(clicked(bool)), this, SLOT(lisaaRivi()));
    connect( ui->poistariviNappi, SIGNAL(clicked(bool)), this, SLOT(poistaRivi()));
    connect( ui->tallennaButton, &QPushButton::clicked, this, [this] { this->tallenna(Tosite::LUONNOS); } );
    connect( ui->valmisNappi, &QPushButton::clicked, this, &KirjausWg::valmis);

    connect( ui->hylkaaNappi, SIGNAL(clicked(bool)), this, SLOT(hylkaa()));

    tyyppiProxy_ = new QSortFilterProxyModel(this);
    tyyppiProxy_->setSourceModel( kp()->tositeTyypit() );
    tyyppiProxy_->setFilterRole( TositeTyyppiModel::LisattavissaRooli);
    tyyppiProxy_->setFilterFixedString("K");
    ui->tositetyyppiCombo->setModel( tyyppiProxy_ );

    connect( ui->tositetyyppiCombo, &QComboBox::currentTextChanged, this, &KirjausWg::vaihdaTositeTyyppi);

    connect( ui->lisaaliiteNappi, &QPushButton::clicked, this, [this]
        { this->lisaaLiite(QFileDialog::getOpenFileName(this, tr("Lisää liite"),QString(),tr("Pdf-tiedosto (*.pdf);;Kuvat (*.png *.jpg);;CSV-tiedosto (*.csv);;Kaikki tiedostot (*.*)"))); });

    connect( ui->avaaNappi, &QPushButton::clicked, this, &KirjausWg::avaaLiite);
    connect( ui->tallennaLiiteNappi, &QPushButton::clicked, this, &KirjausWg::tallennaLiite);
    connect( ui->tulostaLiiteNappi, &QPushButton::clicked, this, &KirjausWg::tulostaLiite);
    connect( ui->poistaLiiteNappi, SIGNAL(clicked(bool)), this, SLOT(poistaLiite()));

    connect( ui->edellinenButton, &QPushButton::clicked, this, [this]() { lataaTosite(this->edellinenSeuraava_.first); });
    connect( ui->seuraavaButton, &QPushButton::clicked, this, [this]() { lataaTosite(this->edellinenSeuraava_.second); });

    // Lisätoimintojen valikko
    QMenu *valikko = new QMenu(this);
    valikko->addAction(QIcon(":/pic/etsi.png"), tr("Siirry tositteeseen\tCtrl+G"), this, SLOT(siirryTositteeseen()));

    valikko->addAction(QIcon(":/pic/tulosta.png"), tr("Tulosta tosite\tCtrl+P"), this, SLOT(tulostaTosite()), QKeySequence("Ctrl+P"));
    uudeksiAktio_ = valikko->addAction(QIcon(":/pic/kopioi.png"), tr("Kopioi uuden pohjaksi\tCtrl+T"), this, SLOT(pohjaksi()), QKeySequence("Ctrl+T"));
    mallipohjaksiAktio_ = valikko->addAction(QIcon(":/pic/uusitiedosto.png"), tr("Tallenna mallipohjaksi"), [this] {this->tosite()->tallenna(Tosite::MALLIPOHJA);});
    poistaAktio_ = valikko->addAction(QIcon(":/pic/roskis.png"),tr("Poista tosite"),this, SLOT(poistaTosite()));    

    tyhjennaViennitAktio_ = valikko->addAction(QIcon(":/pic/edit-clear.png"),tr("Tyhjennä viennit"), [this] { this->tosite()->viennit()->tyhjenna(); if(apuri_) apuri_->reset(); });
    valikko->addAction(QIcon(":/pic/123.png"), tr("Vaihda tunnistenumero"), [this] { this->vaihdaTunniste();});

    ui->valikkoNappi->setMenu( valikko );


    // Enterillä päiväyksestä eteenpäin
    ui->tositePvmEdit->installEventFilter(this);
    ui->otsikkoEdit->installEventFilter(this);

    // ---- tästä alkaen uutta ------    
    ui->viennitView->setTosite(tosite_);
    ui->lokiView->setModel( tosite_->loki() );
    ui->lokiView->horizontalHeader()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
    ui->lokiView->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Stretch);

    ui->liiteView->setLiitteetModel( tosite_->liitteet() );    // Tämä muutettu

    kiertoTab_->hide();
    kommentitTab_->hide();

    connect( tosite_, &Tosite::tilaTieto, this, &KirjausWg::paivita);
    connect( tosite_, &Tosite::talletettu, this, &KirjausWg::tallennettu);
    connect( tosite_, &Tosite::tallennusvirhe, this, &KirjausWg::tallennusEpaonnistui);

    connect( tosite_->liitteet(), &LiitteetModel::valittuVaihtui, this, &KirjausWg::paivitaLiiteNapit);

    connect( ui->viennitView->selectionModel(), SIGNAL(currentRowChanged(QModelIndex,QModelIndex)),
             this, SLOT(vientiValittu()));

    connect( ui->tositePvmEdit, &KpDateEdit::dateChanged, this, [this]  (const QDate& pvm) { this->tosite()->asetaPvm(pvm);} );
    connect( ui->otsikkoEdit, &QLineEdit::textChanged, this, [this] { this->tosite()->setData(Tosite::OTSIKKO, ui->otsikkoEdit->text()); });
    connect( ui->sarjaCombo, &QComboBox::currentTextChanged, this, [this] (const QString& teksti)  { this->tosite()->asetaSarja( teksti); });
    connect( ui->kommentitEdit, &QPlainTextEdit::textChanged, this, [this] { this->tosite()->asetaKommentti(ui->kommentitEdit->toPlainText());});

    connect( ui->lokiView, &QTableView::clicked, this, &KirjausWg::naytaLoki);

    connect( tosite(), &Tosite::pvmMuuttui, this, [this] (const QDate& pvm) { this->ui->tositePvmEdit->setDate(pvm);});
    connect( tosite(), &Tosite::otsikkoMuuttui, this, [this] (const QString& otsikko) { if(otsikko != this->ui->otsikkoEdit->text()) this->ui->otsikkoEdit->setText(otsikko);});
    connect( tosite(), &Tosite::tunnisteMuuttui, this, &KirjausWg::tunnisteVaihtui);
    connect( tosite(), &Tosite::sarjaMuuttui, this, [this] (const QString& sarja) {
        this->ui->sarjaCombo->setCurrentText(sarja);
    });
    connect( tosite(), &Tosite::tyyppiMuuttui, this, &KirjausWg::tositeTyyppiVaihtui);
    connect( tosite(), &Tosite::kommenttiMuuttui, this, &KirjausWg::paivitaKommentti);
    connect( tosite()->liitteet(), &LiitteetModel::liitettaTallennetaan, tosite(), &Tosite::tarkasta );
    connect( tosite()->liitteet(), &LiitteetModel::liitettaTallennetaan, ui->tallennetaanLabel, &QLabel::setVisible );
    connect( tosite()->liitteet(), &LiitteetModel::ocrKaynnissa, ui->ocrLabel, &QLabel::setVisible);

    connect( tosite()->liitteet(), &LiitteetModel::tuonti, this, &KirjausWg::tuonti);
    connect( tosite_, &Tosite::tarkastaSarja, this, &KirjausWg::paivitaSarja);
    connect( kp(), &Kirjanpito::tietokantaVaihtui, this, &KirjausWg::nollaaTietokannanvaihtuessa);
    connect( tosite(), &Tosite::huomioMuuttui, ui->huomioMerkki, &QToolButton::setChecked);
    connect( ui->huomioMerkki, &QToolButton::toggled, tosite(), &Tosite::asetaHuomio);

    // Tilapäisesti poistetaan Varasto
    // Voitaisiin tehdä niinkin, että poistetaan ja lisätään tarvittaessa ;)
    ui->tabWidget->removeTab( ui->tabWidget->indexOf( varastoTab_ ) );


    tosite()->asetaTyyppi( ui->tositetyyppiCombo->currentData(TositeTyyppiModel::KoodiRooli).toInt());
    tosite()->asetaPvm( ui->tositePvmEdit->date());

    connect( kiertoTab_, &KiertoWidget::tallenna, this, &KirjausWg::tallenna);

    connect( ui->muokkaaVientiNappi, &QPushButton::clicked, this, &KirjausWg::muokkaaVientia);
    connect( ui->lisaaVientiNappi, &QPushButton::clicked, this, &KirjausWg::uusiVienti);

//    tosite()->liitteet()->naytaLadattuLiite();
    connect( tosite(), &Tosite::ladattu, this, &KirjausWg::tositeLadattu);

    connect(kommentitTab_, &KommentitWidget::kommentteja, this, &KirjausWg::naytaKommenttimerkki);

}

KirjausWg::~KirjausWg()
{
    // Tallennetaan ruudukon sarakkeiden leveydet
    QStringList leveysLista;
    for(int i=0; i<ui->viennitView->model()->columnCount(); i++)
        leveysLista.append( QString::number( ui->viennitView->horizontalHeader()->sectionSize(i) ) );

    kp()->settings()->setValue("KirjausWgRuudukko", leveysLista);

    delete ui;
}

void KirjausWg::lisaaRivi()
{   
    // Lisätään valinnan jälkeen
    QModelIndex indeksi = tosite_->viennit()->lisaaVienti(ui->viennitView->currentIndex().isValid() ? ui->viennitView->currentIndex().row() + 1 : 0);

    ui->viennitView->setFocus(Qt::TabFocusReason);
    ui->viennitView->setCurrentIndex( indeksi.sibling( indeksi.row(), TositeViennit::TILI )  );

}

void KirjausWg::poistaRivi()
{
    QModelIndex nykyinen = ui->viennitView->currentIndex();
    if( nykyinen.isValid() && nykyinen.sibling(nykyinen.row(), TositeViennit::SELITE).flags() & Qt::ItemIsEditable)
    {
        tosite_->viennit()->removeRows( nykyinen.row(), 1 );
    }
}

void KirjausWg::tyhjenna()
{
    int tyyppi = ui->tositetyyppiCombo->currentData(TositeTyyppiModel::KoodiRooli).toInt();
    if( tyyppi == TositeTyyppi::TILIOTE)
        tyyppi = TositeTyyppi::MENO;
    tosite_->nollaa( ui->tositePvmEdit->date(), tyyppi );
    ui->tabWidget->setCurrentIndex(0);
    ui->tositetyyppiCombo->setFocus();
    ui->tositePvmEdit->setDateRange( kp()->tilitpaatetty().addDays(1), kp()->tilikaudet()->kirjanpitoLoppuu() );

    if( kp()->asetukset()->onko(AsetusModel::AlvVelvollinen))
        ui->viennitView->showColumn(TositeViennit::ALV);
    else
        ui->viennitView->hideColumn(TositeViennit::ALV);
    ui->tallennetaanLabel->hide();
    poistaAktio_->setEnabled(false);
    ui->ocrLabel->hide();
    ui->tabWidget->setTabEnabled(ui->tabWidget->indexOf(ui->tabWidget->findChild<QWidget*>("lokiTab")), false);

    emit naytaPohjat(true);
}

void KirjausWg::valmis()
{
    if( apuri_ )
        apuri_->tositteelle();

    qlonglong debet = 0l;
    qlonglong kredit = 0l;
    for(const auto& vienti: tosite()->viennit()->viennit()) {
        debet +=  vienti.debetSnt();
        kredit += vienti.kreditSnt();
    }

    if( !debet && !kredit && tosite()->tyyppi() != TositeTyyppi::LIITETIETO) {
        if( QMessageBox::question(this, tr("Tositteen tallentaminen"),
                                  tr("Tositteessa ei ole yhtään vientiä.\n"
                                     "Tallennatko tositteen ilman vientejä?")) != QMessageBox::Yes)
            return;
    }

    QString alvTarkastus = tosite_->viennit()->alvTarkastus();
    if( !alvTarkastus.isEmpty()) {
        if( QMessageBox::question(this, tr("Arvonlisäveron kirjaukset"),
                                  tr("Arvonlisäveron kirjauksissa on todennäköisesti virhe.\nTallennetaanko tosite silti?\n%1").arg(alvTarkastus),
                                  QMessageBox::Ok | QMessageBox::Cancel) != QMessageBox::Ok)
            return;
    }


    ui->tallennaButton->setEnabled(false);
    ui->tallennetaanLabel->show();

    tarkastaTuplatJaTallenna(Tosite::KIRJANPIDOSSA);
}

void KirjausWg::hylkaa()
{
    if( ((ui->tallennaButton->isVisible() && ui->tallennaButton->isEnabled()) || (ui->valmisNappi->isVisible() &&  ui->valmisNappi->isEnabled()))
            && tosite()->viennit()->summa() && tosite()->muutettu() ) {
        if( QMessageBox::question(this, tr("Keskeytä kirjaus"),
                                  tosite()->tila() <= Tosite::MALLIPOHJA ?
                                    tr("Haluatko keskeyttää kirjauksen tallentamatta tositetta?") :
                                    tr("Haluatko keskeyttää kirjauksen tallentamatta muutoksia?")
                                  ,
                                  QMessageBox::Yes | QMessageBox::No, QMessageBox::No) != QMessageBox::Yes) {
            return;
        }
    }


    tyhjenna();
    emit tositeKasitelty(false);
}

void KirjausWg::poistaTosite()
{
    if( QMessageBox::question(this, tr("Tositteen poistaminen"),
                              tr("Haluatko todella poistaa tämän tositteen?"),
                              QMessageBox::Yes | QMessageBox::Cancel, QMessageBox::Cancel) == QMessageBox::Yes)
    {
        KpKysely* kysely = kpk(QString("/tositteet/%1").arg( tosite()->data(Tosite::ID).toInt() ), KpKysely::DELETE );
        connect(kysely, &KpKysely::virhe, this, [this] (int /* virhe */, QString selitys) {QMessageBox::critical(this, tr("Tietokantavirhe"),
                                                                        tr("Tietokantavirhe tositetta poistettaessa\n\n%1").arg( selitys ));});
        connect(kysely, &KpKysely::vastaus, this, [this] {this->tyhjenna(); emit this->tositeKasitelty(false); emit kp()->kirjanpitoaMuokattu();});

        kysely->kysy();
    }
}

void KirjausWg::vientiValittu()
{
    QModelIndex index = ui->viennitView->selectionModel()->currentIndex();
    bool muokattavissa =
            index.isValid() &&
            tosite()->viennit()->muokattavissa() &&
            index.data(TositeViennit::TyyppiRooli).toInt() != TositeVienti::ALVKIRJAUS;

    ui->poistariviNappi->setEnabled( muokattavissa);
    ui->muokkaaVientiNappi->setEnabled( muokattavissa );

}

void KirjausWg::uusiVienti()
{
    MuuMuokkausDlg dlg(this);
    dlg.uusi( tosite()->viennit()->uusi( ui->viennitView->currentIndex().row() ) );
    if(dlg.exec() == QDialog::Accepted)
        tosite()->viennit()->lisaa(dlg.vienti());
}

void KirjausWg::muokkaaVientia()
{
    QModelIndex index = ui->viennitView->selectionModel()->currentIndex();
    if(index.isValid() && tosite()->viennit()->muokattavissa()) {
        MuuMuokkausDlg dlg(this);
        TositeVienti vienti = tosite()->viennit()->vienti(index.row());
        dlg.muokkaa(vienti);
        if( dlg.exec() == QDialog::Accepted) {
            tosite()->viennit()->asetaVienti(index.row(), dlg.vienti());
        }
    }
}


void KirjausWg::tulostaTosite()
{
    // Tilapäinen tositteen tulostus
    // Tähän voisi tulla parempi ;)

    kp()->printer()->setPageLayout(QPageLayout(QPageSize(QPageSize::A4), QPageLayout::Landscape, QMargins(10,20,10,10), QPageLayout::Millimeter));

    QPrintDialog printDialog( kp()->printer(), this);



    if( printDialog.exec() )
    {
        Arkistoija arkistoija(kp()->tilikaudet()->tilikausiPaivalle(tosite()->pvm()));               

        QByteArray ba = arkistoija.tositeRunko( tosite()->tallennettava(), true);
        QTextDocument doc;
        QFile css(":/arkisto/print.css");
        css.open(QIODevice::ReadOnly);
        doc.setDefaultStyleSheet( QString::fromUtf8( css.readAll() ) );

        QString teksti = "<div class=kirjanpito>" + kp()->asetukset()->asetus(AsetusModel::OrganisaatioNimi) + "</div>" + QString::fromUtf8(ba);

        doc.setHtml( teksti );
        doc.print( kp()->printer());

    }
}

void KirjausWg::naytaLoki()
{
    NaytinIkkuna *naytin = new NaytinIkkuna();    
    QVariant var = ui->lokiView->currentIndex().data(Qt::UserRole);

    QString data = QString::fromUtf8( QJsonDocument::fromVariant(var).toJson(QJsonDocument::Indented) );
    naytin->nayta(data);
}

void KirjausWg::pohjaksi()
{
    QDialog dlg;
    Ui::KopioiDlg ui;
    ui.setupUi(&dlg);
    ui.pvmEdit->setDate(kp()->paivamaara());
    ui.otsikkoEdit->setText(tosite()->otsikko());
    if( dlg.exec() == QDialog::Accepted) {
        if( apuri_) {
            delete apuri_;
            apuri_ = 0;
        }
        tosite_->pohjaksi( ui.pvmEdit->date(), ui.otsikkoEdit->text(), ui.sailytaErat->isChecked() );
        tositeTyyppiVaihtui( tosite()->tyyppi() );
        tosite()->tarkasta();
    }
}

void KirjausWg::tositeLadattu()
{
    if(tosite()->tila() >= Tosite::SAAPUNUT && tosite()->tila() <= Tosite::HYVAKSYTTY)
        ui->tallennaButton->setText(tr("Tallenna"));

    bool poistoOikeus = kp()->yhteysModel() &&  kp()->yhteysModel()->onkoOikeutta(  tosite()->tila() < Tosite::KIRJANPIDOSSA ? YhteysModel::TOSITE_LUONNOS : YhteysModel::TOSITE_MUOKKAUS  );
    if(  poistoOikeus && tosite()->tila() >= Tosite::KIRJANPIDOSSA ) {
        for(auto vienti : tosite()->viennit()->viennit()) {
            if( vienti.pvm() <= kp()->tilitpaatetty() ||
              ( vienti.alvKoodi() && kp()->alvIlmoitukset()->onkoIlmoitettu(vienti.pvm())  )) {
                poistoOikeus = false;
                break;
            }
        }
    }
    poistaAktio_->setEnabled(poistoOikeus);
}


void KirjausWg::paivita(bool muokattu, int virheet, const Euro &debet, const Euro &kredit)
{
    // Yhdistetty varoitusten näyttäjä
    ui->varoKuva->setPixmap(QPixmap());
    ui->varoTeksti->clear();
    if(kp()->tilitpaatetty() == kp()->tilikaudet()->kirjanpitoLoppuu()) {
        ui->varoKuva->setPixmap(QPixmap(":/pic/stop.png"));
        ui->varoTeksti->setText( tr("Kirjanpidossa ei ole\navointa tilikautta."));
    }else if( virheet & Tosite::PVMLUKITTU )
    {
        ui->varoKuva->setPixmap( QPixmap(":/pic/lukittu.png"));
        ui->varoTeksti->setText( tr("Kirjanpito lukittu\n%1 saakka").arg(kp()->tilitpaatetty().toString("dd.MM.yyyy")));
        virheet |= Tosite::PVMLUKITTU;
    }
    else if( virheet & Tosite::PVMALV )
    {
        ui->varoTeksti->setText( tr("Alv-ilmoitus on jo annettu") );
        ui->varoKuva->setPixmap( QPixmap(":/pic/vero.png"));
    } else if( virheet & Tosite::EITASMAA) {
        ui->varoTeksti->setText( tr("Debet %1    Kredit %2    <b>Erotus %3</b>")
                     .arg(debet.display(true))
                     .arg(kredit.display(true))
                     .arg((debet-kredit).abs().display(true)) );
    } else if( virheet & Tosite::EIAVOINTAKUTTA )
    {
        ui->varoKuva->setPixmap(QPixmap(":/pic/varoitus.png"));
        ui->varoTeksti->setText( tr("Päivämäärälle ei ole\ntilikautta kirjanpidossa."));
    } else if( virheet & Tosite::TILIPUUTTUU) {
        ui->varoTeksti->setText(tr("Tiliöintejä puuttuu"));
        ui->varoKuva->setPixmap(QPixmap(":/pic/oranssi.png"));
    } else if( virheet & Tosite::PVMPUUTTUU) {
        ui->varoTeksti->setText(tr("Päivämääriä puuttuu"));
        ui->varoKuva->setPixmap(QPixmap(":/pic/varoitus.png"));
    }  else if( debet ) {
        ui->varoTeksti->setText( tr("Summa %L1 €").arg(debet,0,'f',2) );
    }


    // Nappien enablointi
    // Täällä pitäisi olla jossain myös oikeuksien tarkastus ;)
    mallipohjaksiAktio_->setEnabled( tosite()->data(Tosite::TILA).toInt() <= Tosite::MALLIPOHJA && kp()->yhteysModel() && kp()->yhteysModel()->onkoOikeutta(YhteysModel::TOSITE_LUONNOS));
    ui->tallennaButton->setVisible( tosite()->data(Tosite::TILA).toInt() < Tosite::KIRJANPIDOSSA && kp()->yhteysModel() && kp()->yhteysModel()->onkoOikeutta(YhteysModel::TOSITE_LUONNOS));
    ui->tallennaButton->setEnabled( muokattu && !tosite()->liitteet()->tallennetaanko() && kp()->yhteysModel() );
    ui->valmisNappi->setVisible( kp()->yhteysModel() && kp()->yhteysModel()->onkoOikeutta(YhteysModel::TOSITE_MUOKKAUS));

    const int tila = tosite()->tila();
    const int tyyppi = tosite()->tyyppi();

    const bool valmisSallittu = (muokattu || ( tila > Tosite::POISTETTU && tila < Tosite::KIRJANPIDOSSA )) &&
                          (!virheet || (virheet == Tosite::PVMALV && tosite()->id() ) || ( tyyppi == TositeTyyppi::TILIOTE && virheet == Tosite::TILIPUUTTUU && qobject_cast<PilviModel*>(kp()->yhteysModel()))) &&
                          !tosite()->liitteet()->tallennetaanko();

    ui->valmisNappi->setEnabled( valmisSallittu );

    uudeksiAktio_->setEnabled( !muokattu );

    salliMuokkaus( tosite()->tila() < Tosite::KIRJANPIDOSSA && (tosite_->tyyppi() < TositeTyyppi::MYYNTILASKU || tosite_->tyyppi() > TositeTyyppi::MAKSUMUISTUTUS) ?
                       Sallittu :
                       ( virheet & Tosite::PVMLUKITTU ? Lukittu : (virheet & Tosite::PVMALV ? AlvLukittu : Sallittu) ));
    if( muokattu )
        emit kp()->piilotaTallennusWidget();

    emit naytaPohjat(!tosite()->liitteet()->rowCount() && !tosite()->id());
}

void KirjausWg::tallenna(int tilaan)
{
    int tila = tosite()->tila();
    if( tila == Tosite::MALLIPOHJA && tilaan != Tosite::MALLIPOHJA) {
        tosite()->setData(Tosite::ID, 0);
    } else if(tilaan == Tosite::LUONNOS && tila >= Tosite::SAAPUNUT && tila <= Tosite::HYVAKSYTTY) {
        tilaan = tila;      // Kierron luonnokset jäävät paikalleen ;)
    }

    if( tilaan == Tosite::MALLIPOHJA) {
        tosite()->setData(Tosite::TILA, Tosite::MALLIPOHJA);
    }

    ui->tallennaButton->setEnabled(false);
    kp()->odotusKursori(true);
    ui->tallennetaanLabel->show();
    if(apuri_)
        apuri_->tositteelle();

    tarkastaTuplatJaTallenna(tilaan);
}

void KirjausWg::tallennettu(int /* id */, int tunniste, const QDate &pvm, const QString& sarja, int tila)
{
    if( ui->tositetyyppiCombo->currentData(TositeTyyppiModel::KoodiRooli) == TositeTyyppi::TILIOTE)
        ui->tositetyyppiCombo->setCurrentIndex(0);

    kp()->odotusKursori(false);
    emit kp()->tositeTallennettu(tunniste, pvm, sarja, tila);
    tyhjenna();
    emit tositeKasitelty(true);
}

void KirjausWg::tallennusEpaonnistui(int virhe)
{
    QMessageBox::critical(this, tr("Tallennus epäonnistui"), tr("Tositteen tallentaminen epäonnistui (Virhe %1)").arg(virhe));
    ui->tallennetaanLabel->hide();
    ui->tallennaButton->setEnabled(true);
}

void KirjausWg::tuonti(const QVariantMap& map)
{    
    if( tosite()->tila() >= Tosite::KIRJANPIDOSSA )
        return;

    if( map.contains("tyyppi") && !tosite()->viennit()->summa() && tosite()->tila() != Tosite::MALLIPOHJA)
        ui->tositetyyppiCombo->setCurrentIndex( ui->tositetyyppiCombo->findData( map.value("tyyppi") ) );
    if( map.value("tositepvm").toDate().isValid() && !tosite()->viennit()->summa()) {
        ui->tositePvmEdit->setDate( map.value("tositepvm").toDate() );
    }
    if( apuri_) {
        apuri_->tuo(map);
        const QDate& laskupvm = map.value("lasku").toMap().value("pvm").toDate();
        if( laskupvm.isValid())
            tosite()->asetaPvm(laskupvm);
    }

    else if( map.value("tyyppi") == TositeTyyppi::TUONTI) {
        if(map.contains("pvm"))
            tosite()->asetaPvm(map.value("pvm").toDate());
        if(map.contains("otsikko"))
            tosite()->asetaOtsikko(map.value("otsikko").toString());
        for(auto &vienti : map.value("viennit").toList()) {
            tosite()->viennit()->lisaa( vienti.toMap());
        }
    }
}

void KirjausWg::nollaaTietokannanvaihtuessa()
{
    ui->tositetyyppiCombo->setCurrentIndex(0);
    tositeTyyppiVaihtui(TositeTyyppi::MENO);
    tyhjenna();
    QDate pvm = kp()->paivamaara();
    if( pvm > kp()->tilikaudet()->kirjanpitoLoppuu())
        pvm = kp()->tilikaudet()->kirjanpitoLoppuu();
    tosite()->asetaPvm(pvm);
    ui->sarjaCombo->clear();
    ui->sarjaCombo->addItems(kp()->tositeSarjat());

    int kommentitIndex = ui->tabWidget->indexOf(kommentitTab_);
    bool pilvessa = qobject_cast<PilviModel*>(kp()->yhteysModel());
    if( pilvessa && kommentitIndex < 0)
        ui->tabWidget->insertTab( ui->tabWidget->count()-1, kommentitTab_, QIcon(":/pic/kupla-harmaa.png"), tr("Kommentit") );
    else if( !pilvessa && kommentitIndex > 0)
        ui->tabWidget->removeTab(kommentitIndex);

}


void KirjausWg::naytaKommenttimerkki(bool onko)
{
    int indeksi = ui->tabWidget->indexOf(kommentitTab_);
    if( indeksi > 0) {
        ui->tabWidget->setTabIcon(indeksi, QIcon(onko ? ":/pic/kupla.png" : ":/pic/kupla-harmaa.png"));
    }
}

void KirjausWg::vaihdaTunniste()
{
    bool ok;
    int tunniste = QInputDialog::getInt(this, tr("Valitse tositenumero"), tr("Uusi tositenumero:\n" "Ohjelma ei tarkasta tositenumeroa!"),
                                        tosite()->tunniste(), 1, 999999, 1, &ok);
    if( ok ) {
        tosite()->setData(Tosite::TUNNISTE, tunniste);
        tunnisteVaihtui(tunniste);
    }
}

void KirjausWg::siirryTositteeseen()
{
    int id = SiirryDlg::tositeId( ui->tositePvmEdit->date(), QString() );

    if( id )
    {
        if( ui->tallennaButton->isEnabled() )
        {
            if( QMessageBox::question(this, tr("Kitsas"), tr("Nykyistä kirjausta on muokattu. Siirrytkö toiseen tositteeseen tallentamatta tekemiäsi muutoksia?")) != QMessageBox::Yes)
            {
                return;
            }
        }
        lataaTosite(id);
    }
}



bool KirjausWg::eventFilter(QObject *watched, QEvent *event)
{
    if( watched == ui->tositePvmEdit || watched == ui->otsikkoEdit || watched == ui->tositetyyppiCombo)
    {
        if( event->type() == QEvent::KeyPress)
        {
            QKeyEvent *keyEvent = static_cast<QKeyEvent*>(event);
            if( keyEvent->key() == Qt::Key_Enter ||
                keyEvent->key() == Qt::Key_Return)
            {
                focusNextChild();
                return true;
            }
            // Otsikosta pääsee tabulaattorilla uudelle riville
            else if( keyEvent->key() == Qt::Key_Tab && watched == ui->otsikkoEdit )
            {
                if( apuri_) {
                    apuri_->otaFokus();
                    return true;
                }

                if( !tosite_->viennit()->rowCount() )
                    lisaaRivi();
                ui->viennitView->setFocus(Qt::TabFocusReason);
                ui->viennitView->setCurrentIndex( ui->viennitView->model()->index(0,TositeViennit::TILI));
                return true;
            }
        }
    }
    return QWidget::eventFilter(watched, event);
}

void KirjausWg::paivitaLiiteNapit()
{
    bool valittu = tosite()->liitteet()->naytettavaIndeksi() > -1;


    ui->poistaLiiteNappi->setEnabled(valittu);
    ui->avaaNappi->setEnabled(valittu);
    ui->tallennaLiiteNappi->setEnabled(valittu);
    ui->tulostaLiiteNappi->setEnabled(valittu && tosite()->liitteet()->naytettava().data(LiitteetModel::TyyppiRooli).toString() != "application/octet-stream");

    if( tosite_->liitteet()->rowCount() )
        ui->tabWidget->setTabIcon( ui->tabWidget->indexOf(liitteetTab_) , QIcon(":/pic/liite-aktiivinen.png"));
    else
        ui->tabWidget->setTabIcon( ui->tabWidget->indexOf(liitteetTab_), QIcon(":/pic/liite"));
}

void KirjausWg::tarkastaTuplatJaTallenna(int tila)
{
    if( !tosite()->kumppani() ) {
        tosite()->tallenna(tila);
    } else {
        KpKysely *kysely = kpk("/tositteet");
        kysely->lisaaAttribuutti("kumppani", tosite()->kumppani());
        kysely->lisaaAttribuutti("laskupvm",tosite()->laskupvm());
        kysely->lisaaAttribuutti("tyyppi", tosite()->tyyppi());
        if( !tosite()->laskuNumero().isEmpty())
            kysely->lisaaAttribuutti("laskunumero", tosite()->laskuNumero());
        if( !tosite()->viite().isEmpty())
            kysely->lisaaAttribuutti("viite", tosite()->viite());
        connect(kysely, &KpKysely::vastaus, this, [this, tila] (QVariant* data)  { this->tuplaTietoSaapuu(data, tila);});
        kysely->kysy();
    }
}

void KirjausWg::tuplaTietoSaapuu(QVariant *data, int tila)
{
    QVariantList lista = data->toList();
    QString tuplat;

    Euro summa = tosite()->viennit()->summa();

    for(const auto& item : qAsConst(lista)) {
        QVariantMap map = item.toMap();
        if( map.value("id").toInt() == tosite()->id()) {
            // Sallii aina saman tositteen muokkaamisen
            tosite()->tallenna(tila);
            return;
        }
        if( Euro(map.value("summa").toString()) != summa)
            continue;
        tuplat.append(QString("%1 %2 \n")
                      .arg(kp()->tositeTunnus(map.value("tunniste").toInt(),
                                              map.value("pvm").toDate(),
                                              map.value("sarja").toString(), true),
                      map.value("otsikko").toString()));
    }

    if( !tuplat.isEmpty()) {
        ui->tallennetaanLabel->hide();
        int vastaus = QMessageBox::question(this, tr("Tosite saattaa olla jo kirjattu"),
                              tr("Kirjanpidosta löytyy jo samankaltainen tosite \n\n%1 \n"
                                 "Tallennetaanko tosite silti?").arg(tuplat),
                              QMessageBox::Yes | QMessageBox::No | QMessageBox::Cancel, QMessageBox::Cancel);
        if( vastaus == QMessageBox::No)
            tyhjenna();
        if( vastaus != QMessageBox::Yes)
            return;
    }
    tosite()->tallenna(tila);
}


void KirjausWg::lataaTosite(int id)
{
    kp()->odotusKursori(true);
    tosite_->lataa(id);
    ui->idLabel->setText(QString::number(id));
    ui->tabWidget->setTabEnabled(ui->tabWidget->indexOf(ui->tabWidget->findChild<QWidget*>("lokiTab")), true);
    emit naytaPohjat(false);    
}

void KirjausWg::paivitaKommentti(const QString &kommentti)
{
    int kommenttiIndeksi = ui->tabWidget->indexOf(memoTab_);

    if( kommentti.isEmpty())
    {
        ui->tabWidget->setTabIcon(kommenttiIndeksi, QIcon(":/pic/kommentti-harmaa.png"));
    }
    else
    {
        ui->tabWidget->setTabIcon(kommenttiIndeksi, QIcon(":/pic/kommentti.png"));
    }

    if( kommentti != ui->kommentitEdit->toPlainText())
        ui->kommentitEdit->setPlainText( kommentti );

}

void KirjausWg::lisaaLiite(const QString& polku)
{
    if( !polku.isEmpty())
    {
        tosite()->liitteet()->lisaaHetiTiedosto(polku);
        // Valitsee lisätyn liitteen
        ui->liiteView->setCurrentIndex( tosite()->liitteet()->index( tosite()->liitteet()->rowCount() - 1 ) );
        paivitaLiiteNapit();
    }

}


void KirjausWg::lisaaLiiteDatasta(const QByteArray &data, const QString &nimi)
{
     tosite()->liitteet()->lisaaHeti(data, nimi);
      ui->liiteView->setCurrentIndex( tosite()->liitteet()->index( tosite()->liitteet()->rowCount() - 1 ) );
     paivitaLiiteNapit();

}

void KirjausWg::salliMuokkaus(MuokkausSallinta sallitaanko)
{
    ui->tositePvmEdit->setEnabled(sallitaanko == Sallittu);
    ui->tositetyyppiCombo->setEnabled(sallitaanko == Sallittu);
    ui->kommentitEdit->setEnabled(sallitaanko != Lukittu);
    ui->otsikkoEdit->setEnabled(sallitaanko != Lukittu);
    ui->lisaaliiteNappi->setEnabled(sallitaanko != Lukittu);
    ui->poistaLiiteNappi->setEnabled(sallitaanko == Sallittu);
    ui->lisaaRiviNappi->setEnabled(sallitaanko == Sallittu);
    ui->lisaaVientiNappi->setEnabled(sallitaanko == Sallittu);
    ui->huomioMerkki->setEnabled( sallitaanko != Lukittu);

    if(sallitaanko == Sallittu)
        ui->tositePvmEdit->setDateRange( kp()->tilitpaatetty().addDays(1), kp()->tilikaudet()->kirjanpitoLoppuu() );
    else
        ui->tositePvmEdit->setDateRange( kp()->tilikaudet()->kirjanpitoAlkaa(), kp()->tilikaudet()->kirjanpitoLoppuu() );

    tosite_->viennit()->asetaMuokattavissa( sallitaanko == Sallittu && !apuri_ );    

    ui->lisaaRiviNappi->setVisible( !apuri_);
    ui->lisaaVientiNappi->setVisible(!apuri_);
    ui->muokkaaVientiNappi->setVisible(!apuri_);
    ui->poistariviNappi->setVisible( !apuri_);

    if( apuri_ ) {
        apuri_->salliMuokkaus(sallitaanko == Sallittu);
    }
}

void KirjausWg::vaihdaTositeTyyppi()
{
    tosite()->asetaTyyppi( ui->tositetyyppiCombo->currentData(TositeTyyppiModel::KoodiRooli).toInt() );
}

void KirjausWg::tositeTyyppiVaihtui(int tyyppiKoodi)
{
    // Tässä voisi laittaa muutenkin apurit paikalleen
    if( apuri_ )
    {
        ui->tabWidget->removeTab( ui->tabWidget->indexOf( apuri_) );
        delete apuri_;
    }
    apuri_ = nullptr;

    // Liitetiedoilla ei ole vientejä
    ui->tabWidget->setTabEnabled( ui->tabWidget->indexOf(viennitTab_) , tyyppiKoodi != TositeTyyppi::LIITETIETO);

    // Varasto ei toistaiseksi käytössä
    // ui->tabWidget->setTabEnabled( ui->tabWidget->indexOf(varastoTab_), false);

    if( tyyppiKoodi == TositeTyyppi::TULO || tyyppiKoodi == TositeTyyppi::MENO
            || tyyppiKoodi == TositeTyyppi::KULULASKU || tyyppiKoodi == TositeTyyppi::SAAPUNUTVERKKOLASKU)
    {
        apuri_ = new TuloMenoApuri(this, tosite_);
    } else if( tyyppiKoodi == TositeTyyppi::SIIRTO) {
        apuri_ = new SiirtoApuri(this, tosite_);
    } else if( tyyppiKoodi == TositeTyyppi::TILIOTE ) {
        apuri_ = new TilioteApuri(this, tosite_);
    } else if( tyyppiKoodi == TositeTyyppi::PALKKA) {
        apuri_ = new PalkkaApuri(this, tosite_);
    }

    bool lisattavatyyppi = kp()->tositeTyypit()->onkolisattavissa(tyyppiKoodi);
    if( lisattavatyyppi )
        ui->tositetyyppiCombo->setCurrentIndex( ui->tositetyyppiCombo->findData( tosite_->data(Tosite::TYYPPI).toInt(), TositeTyyppiModel::KoodiRooli ) );
    else
        ui->tositetyyppiLabel->setText( kp()->tositeTyypit()->nimi(tyyppiKoodi) );
    ui->tositetyyppiCombo->setVisible( lisattavatyyppi );
    ui->tositetyyppiLabel->setVisible( !lisattavatyyppi );

    paivitaSarja();

    if( apuri_)
    {
        ui->tabWidget->insertTab(0, apuri_, QIcon(":/pic/apuri64.png"), tr("Kirjaa"));
        ui->tabWidget->setCurrentIndex(0);
        apuri_->reset();                
    }

    if( tyyppiKoodi == TositeTyyppi::LIITETIETO)
        ui->tabWidget->setCurrentIndex(1);
    else
        ui->tabWidget->setCurrentIndex(0);

    if( tyyppiKoodi == TositeTyyppi::MUU && !tosite_->viennit()->rowCount())
        lisaaRivi();

    if( ui->otsikkoEdit->text().startsWith("Tiliote") && tyyppiKoodi != TositeTyyppi::TILIOTE && !tosite()->resetoidaanko())
        ui->otsikkoEdit->clear();
}

void KirjausWg::tunnisteVaihtui(int tunniste)
{
    if(tunniste)
        ui->tunnisteLabel->setText(QString::number(tunniste));
    else if( tosite()->tositetila() == 0 )
        ui->tunnisteLabel->setText(tr("Uusi tosite"));
    else
        ui->tunnisteLabel->setText( Tosite::tilateksti(tosite()->tositetila()) );
    QString sarja = tosite()->sarja();
    ui->sarjaLabel->setVisible( kp()->asetukset()->onko(AsetusModel::EriSarjaan) || kp()->asetukset()->onko(AsetusModel::KateisSarjaan) || !sarja.isEmpty() );
    ui->sarjaCombo->setVisible( kp()->asetukset()->onko(AsetusModel::EriSarjaan) || kp()->asetukset()->onko(AsetusModel::KateisSarjaan) || !sarja.isEmpty() );

    if( selaus_ && tosite_->id()) {
        edellinenSeuraava_ = selaus_->edellinenSeuraava( tosite_->id() );
        if( edellinenSeuraava_.first) {
            kp()->liiteCache()->tositteenLiitteidenEnnakkoHaku( edellinenSeuraava_.first );
        }
        if( edellinenSeuraava_.second) {
            kp()->liiteCache()->tositteenLiitteidenEnnakkoHaku( edellinenSeuraava_.second );
        }
    } else {
        edellinenSeuraava_ = qMakePair(0,0);
    }

    if( tunniste ) {
        ui->vuosiLabel->setVisible(true);
        ui->edellinenButton->setVisible(true);
        ui->tallennaButton->setVisible(false);

        ui->tunnisteLabel->setText( QString::number( tunniste ) );
        ui->vuosiLabel->setText( kp()->tilikaudet()->tilikausiPaivalle( tosite()->pvm() ).pitkakausitunnus() );

        if( selaus_) {
            ui->seuraavaButton->setVisible(true);
            ui->tunnisteLabel->setVisible(true);
            ui->edellinenButton->setEnabled( edellinenSeuraava_.first );
            ui->seuraavaButton->setEnabled( edellinenSeuraava_.second );
        }

    } else {
        ui->edellinenButton->setVisible(false);
        ui->seuraavaButton->setVisible(false);
        ui->vuosiLabel->setVisible(false);
    }

    int kiertoIndex = ui->tabWidget->indexOf(kiertoTab_);
    if( kp()->kierrot()->rowCount() && ui->tabWidget->count()) {
        if( kiertoIndex < 0)
            kiertoIndex = ui->tabWidget->insertTab( ui->tabWidget->count()-1, kiertoTab_, QIcon(":/pic/kierto.svg"), tr("Kierto") );

        if( tosite()->tositetila() >= Tosite::HYLATTY && tosite()->tositetila() <= Tosite::HYVAKSYTTY)
        {
            ui->tabWidget->setCurrentWidget(kiertoTab_);
            ui->tabWidget->setTabIcon( kiertoIndex, QIcon(":/pic/kierto.svg") );
        } else {
            ui->tabWidget->setTabIcon( kiertoIndex, QIcon(":/pic/kierto-harmaa.svg") );
        }
    } else if( kiertoIndex >= 0) {
        ui->tabWidget->removeTab(kiertoIndex);
    }

}

void KirjausWg::paivitaSarja(bool kateinen)
{
    if( (kp()->asetukset()->onko(AsetusModel::EriSarjaan) || kp()->asetukset()->onko(AsetusModel::KateisSarjaan)) &&
         !tosite()->id()   )
        tosite()->asetaSarja( kp()->tositeTyypit()->sarja( tosite_->tyyppi(), kateinen ) ) ;
}


void KirjausWg::poistaLiite()
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

