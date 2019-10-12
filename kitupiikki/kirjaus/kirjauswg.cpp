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

#include <QShortcut>
#include <QSettings>

#include <QSortFilterProxyModel>
#include <QCompleter>

#include <QClipboard>
#include <QMimeData>

#include "kirjauswg.h"
#include "tilidelegaatti.h"
#include "eurodelegaatti.h"
#include "pvmdelegaatti.h"
#include "kohdennusdelegaatti.h"

#include "verodialogi.h"

#include "siirrydlg.h"

#include "db/kirjanpito.h"

#include "tuonti/vanhatuonti.h"
#include "ui_numerosiirto.h"
#include "naytin/naytinikkuna.h"
#include "ui_kopioitosite.h"

#include "db/tositetyyppimodel.h"

#include "apuri/tulomenoapuri.h"
#include "apuri/siirtoapuri.h"
#include "apuri/tilioteapuri.h"
#include "apuri/palkkaapuri.h"
#include "model/tosite.h"
#include "model/tositeliitteet.h"
#include "model/tositeviennit.h"
#include "model/tositeloki.h"
#include "tallennettuwidget.h"


KirjausWg::KirjausWg( QWidget *parent)
    : QWidget(parent),
      tosite_( new Tosite(this)),
      apuri_(nullptr),
      tallennettuWidget_( new TallennettuWidget(this) )
{
    ui = new Ui::KirjausWg();
    ui->setupUi(this);

    viennitTab_ = ui->tabWidget->widget(VIENNIT);
    kommentitTab_ = ui->tabWidget->widget(KOMMENTIT);
    liitteetTab_ = ui->tabWidget->widget(LIITTEET);
    varastoTab_ = ui->tabWidget->widget(VARASTO);
    lokiTab_ = ui->tabWidget->widget(LOKI);


    // Tämä pitää säilyttää, jotta saadaan päivämäärä paikalleen
    ui->viennitView->setItemDelegateForColumn( TositeViennit::PVM, new PvmDelegaatti(ui->tositePvmEdit));

    connect( ui->lisaaRiviNappi, SIGNAL(clicked(bool)), this, SLOT(lisaaRivi()));
    connect( ui->poistariviNappi, SIGNAL(clicked(bool)), this, SLOT(poistaRivi()));
    connect( ui->tallennaButton, &QPushButton::clicked, [this] {  if(apuri_) apuri_->tositteelle(); this->tosite_->tallenna(Tosite::LUONNOS); } );
    connect( ui->valmisNappi, &QPushButton::clicked, [this] { if(apuri_) apuri_->tositteelle(); this->tosite_->tallenna(Tosite::KIRJANPIDOSSA);} );

    connect( ui->hylkaaNappi, SIGNAL(clicked(bool)), this, SLOT(hylkaa()));
    connect( ui->kommentitEdit, SIGNAL(textChanged()), this, SLOT(paivitaKommenttiMerkki()));

    tyyppiProxy_ = new QSortFilterProxyModel(this);
    tyyppiProxy_->setSourceModel( kp()->tositeTyypit() );
    tyyppiProxy_->setFilterRole( TositeTyyppiModel::LisattavissaRooli);
    tyyppiProxy_->setFilterFixedString("K");
    ui->tositetyyppiCombo->setModel( tyyppiProxy_ );


    connect( ui->tositePvmEdit, SIGNAL(editingFinished()), this, SLOT(pvmVaihtuu()));
    connect( ui->tositetyyppiCombo, SIGNAL(currentIndexChanged(int)), this, SLOT(vaihdaTositeTyyppi()));

    connect( ui->lisaaliiteNappi, SIGNAL(clicked(bool)), this, SLOT(lisaaLiite()));
    connect( ui->avaaNappi, &QPushButton::clicked, this, &KirjausWg::avaaLiite);
    connect( ui->tulostaLiiteNappi, &QPushButton::clicked, this, &KirjausWg::tulostaLiite);
    connect( ui->poistaLiiteNappi, SIGNAL(clicked(bool)), this, SLOT(poistaLiite()));


    // Lisätoimintojen valikko
    QMenu *valikko = new QMenu(this);
    valikko->addAction(QIcon(":/pic/etsi.png"), tr("Siirry tositteeseen\tCtrl+G"), this, SLOT(siirryTositteeseen()));
    valikko->addAction(QIcon(":/pic/tulosta.png"), tr("Tulosta tosite\tCtrl+P"), this, SLOT(tulostaTosite()), QKeySequence("Ctrl+P"));
    uudeksiAktio_ = valikko->addAction(QIcon(":/pic/kopioi.png"), tr("Kopioi uuden pohjaksi\tCtrl+T"), this, SLOT(uusiPohjalta()), QKeySequence("Ctrl+T"));
    poistaAktio_ = valikko->addAction(QIcon(":/pic/roskis.png"),tr("Poista tosite"),this, SLOT(poistaTosite()));
//    tyhjennaViennitAktio_ = valikko->addAction(QIcon(":/pic/edit-clear.png"),tr("Tyhjennä viennit"), model_->vientiModel(), &VientiModel::tyhjaa);

    ui->valikkoNappi->setMenu( valikko );


    // Enterillä päiväyksestä eteenpäin
    ui->tositePvmEdit->installEventFilter(this);
    ui->otsikkoEdit->installEventFilter(this);
    ui->tositetyyppiCombo->installEventFilter(this);


    // ---- tästä alkaen uutta ------

    tosite_ = new Tosite();
    ui->viennitView->setModel( tosite_->viennit() );
    ui->lokiView->setModel( tosite_->loki() );
    ui->lokiView->horizontalHeader()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
    ui->lokiView->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Stretch);

    ui->liiteView->setModel( tosite_->liitteet() );
    ui->liiteView->setDropIndicatorShown(true);

    connect( tosite_, &Tosite::tila, this, &KirjausWg::paivita);
    connect( tosite_, &Tosite::talletettu, this, &KirjausWg::tallennettu);
    connect( tosite_, &Tosite::tallennusvirhe, this, &KirjausWg::tallennusEpaonnistui);
    connect( tosite_, &Tosite::ladattu, this, &KirjausWg::tiedotModelista);

    connect( tosite_->liitteet(), &TositeLiitteet::naytaliite, this, &KirjausWg::liiteValittu);

    vaihdaTositeTyyppi();

    connect( ui->liiteView->selectionModel(), SIGNAL(currentChanged(QModelIndex,QModelIndex)),
             this, SLOT(liiteValinta(QModelIndex)));
    connect( ui->viennitView->selectionModel(), SIGNAL(currentRowChanged(QModelIndex,QModelIndex)),
             this, SLOT(vientiValittu()));
    connect( ui->viennitView, SIGNAL(activated(QModelIndex)), this, SLOT( vientivwAktivoitu(QModelIndex)));
    connect( ui->otsikkoEdit, &QLineEdit::textChanged, [this] { this->tosite()->setData(Tosite::OTSIKKO, ui->otsikkoEdit->text()); });
    connect( ui->sarjaEdit, &QLineEdit::textChanged, [this] { this->tosite()->setData(Tosite::SARJA, ui->sarjaEdit->text()); });

    connect( tosite_, &Tosite::otsikkoMuuttui, [this] (const QString& otsikko) { if( otsikko != ui->otsikkoEdit->text()) this->ui->otsikkoEdit->setText(otsikko); });

    // Tagivalikko
    ui->viennitView->viewport()->installEventFilter(this);
    ui->viennitView->setFocusPolicy(Qt::StrongFocus);

    connect( tosite()->liitteet(), &TositeLiitteet::tuonti, this, &KirjausWg::tuonti);

    connect( tosite_, &Tosite::tarkastaSarja, this, &KirjausWg::paivitaSarja);


    // Tilapäisesti poistetaan Varasto
    // Voitaisiin tehdä niinkin, että poistetaan ja lisätään tarvittaessa ;)
    ui->tabWidget->removeTab( ui->tabWidget->indexOf( varastoTab_ ) );
}

KirjausWg::~KirjausWg()
{
    // Tallennetaan ruudukon sarakkeiden leveydet
    QStringList leveysLista;
    for(int i=0; i<ui->viennitView->model()->columnCount(); i++)
        leveysLista.append( QString::number( ui->viennitView->horizontalHeader()->sectionSize(i) ) );

    kp()->settings()->setValue("KirjausWgRuudukko", leveysLista);

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
    tosite_->nollaa( ui->tositePvmEdit->date(), ui->tositetyyppiCombo->currentData(TositeTyyppiModel::KoodiRooli).toInt() );
    tiedotModelista();
    ui->tallennaButton->setVisible(true);
    ui->tabWidget->setCurrentIndex(0);
    ui->tositetyyppiCombo->setFocus();
    ui->sarjaEdit->setVisible( kp()->asetus("sarjaan").toInt() );
    ui->sarjaLabel->setVisible( kp()->asetus("sarjaan").toInt() );
}

void KirjausWg::tallenna()
{
    // Luonnokselle tehtävä oma slottinsa??? Tai sitten vähän lamdbaa peliin ;)
    tosite_->tallenna();
    return;
}

void KirjausWg::hylkaa()
{
    tyhjenna();
    emit tositeKasitelty();
}

void KirjausWg::poistaTosite()
{
    if( QMessageBox::question(this, tr("Tositteen poistaminen"),
                              tr("Haluatko todella poistaa tämän tositteen?"),
                              QMessageBox::Yes | QMessageBox::Cancel, QMessageBox::Cancel) == QMessageBox::Yes)
    {
        KpKysely* kysely = kpk(QString("/tositteet/%1").arg( tosite()->data(Tosite::ID).toInt() ), KpKysely::PATCH );
        QVariantMap map;
        map.insert("tila", Tosite::POISTETTU);

        connect(kysely, &KpKysely::virhe, [this] (int /* virhe */, QString selitys) {QMessageBox::critical(this, tr("Tietokantavirhe"),
                                                                        tr("Tietokantavirhe tositetta poistettaessa\n\n%1").arg( selitys ));});
        connect(kysely, &KpKysely::vastaus, [this] {this->tyhjenna(); emit this->tositeKasitelty();});

        kysely->kysy(map);
    }
}

void KirjausWg::vientiValittu()
{
    QModelIndex index = ui->viennitView->selectionModel()->currentIndex();
    ui->poistariviNappi->setEnabled( index.isValid() );

    qDebug() << index.row() << " ---- valittu ---";
}

void KirjausWg::uusiPohjalta()
{
    Ui::KopioiDlg kui;
    QDialog dlg;

    kui.setupUi(&dlg);
    kui.otsikkoEdit->setText( ui->otsikkoEdit->text() );
    kui.pvmEdit->setDate( kp()->paivamaara() );
    kui.pvmEdit->setDateRange( kp()->tilitpaatetty().addDays(1), kp()->tilikaudet()->kirjanpitoLoppuu() );

    if( dlg.exec() == QDialog::Accepted)
    {
//        model_->uusiPohjalta( kui.pvmEdit->date(), kui.otsikkoEdit->text() );
        tiedotModelista();
        emit liiteValittu(QByteArray());
    }

}

void KirjausWg::vientivwAktivoitu(QModelIndex indeksi)
{
    // Tehdään alv-kirjaus
    if( 1 )     // Tarkastettava vielä, onko sallittu
    {

        if(indeksi.column() == TositeViennit::ALV )
        {
            VeroDialogi verodlg(this);
            if( verodlg.nayta( indeksi.data(TositeViennit::AlvKoodiRooli).toInt(), indeksi.data(TositeViennit::AlvProsenttiRooli).toInt() ))
            {
                tosite()->viennit()->setData(indeksi, verodlg.alvKoodi() , TositeViennit::AlvKoodiRooli);
                tosite()->viennit()->setData(indeksi, verodlg.alvProsentti() , TositeViennit::AlvProsenttiRooli);
            }
        }
        else if(indeksi.column() == TositeViennit::KOHDENNUS && indeksi.data(TositeViennit::TaseErittelyssaRooli).toBool())
        {
//            TaseEraValintaDialogi dlg(this);
//            dlg.nayta( model_->vientiModel(), indeksi );
        }
    }
}


void KirjausWg::tulostaTosite()
{
    QPrintDialog printDialog( kp()->printer(), this);
    if( printDialog.exec() )
    {
        QPainter painter( kp()->printer() );
//        model()->tuloste().tulosta( kp()->printer(), &painter );
        painter.end();
    }
}

void KirjausWg::naytaSelvitys()
{
    NaytinIkkuna *naytin = new NaytinIkkuna();
//    naytin->naytaRaportti( model()->selvittelyTuloste() );
}

void KirjausWg::valmis()
{
    tosite_->setData(Tosite::TILA, 100);
    tallenna();
}

void KirjausWg::paivita(bool muokattu, int virheet, double debet, double kredit)
{
    // Yhdistetty varoitusten näyttäjä
    ui->varoKuva->setPixmap(QPixmap());
    ui->varoTeksti->clear();

    if( kp()->tilitpaatetty() >= kp()->tilikaudet()->kirjanpitoLoppuu() )
    {
        ui->varoKuva->setPixmap(QPixmap(":/pic/stop.png"));
        ui->varoTeksti->setText( tr("Kirjanpidossa ei ole\navointa tilikautta."));
    }
    else if( virheet & Tosite::PVMLUKITTU || kp()->tilitpaatetty() >= ui->tositePvmEdit->date())
    {
        ui->varoKuva->setPixmap( QPixmap(":/pic/lukittu.png"));
        ui->varoTeksti->setText( tr("Kirjanpito lukittu\n%1 saakka").arg(kp()->tilitpaatetty().toString("dd.MM.yyyy")));
    }
    else if( virheet & Tosite::PVMALV )
    {
        ui->varoTeksti->setText( tr("Alv-ilmoitus annettu\n%1 saakka").arg(kp()->asetukset()->pvm("AlvIlmoitus").toString("dd.MM.yyyy")));
        ui->varoKuva->setPixmap( QPixmap(":/pic/vero.png"));
    } else if( virheet & Tosite::EITASMAA) {
        ui->varoTeksti->setText( tr("Debet %L1 €    Kredit %L2 €    <b>Erotus %L3 €</b>")
                     .arg(debet,0,'f',2)
                     .arg(kredit,0,'f',2)
                     .arg(qAbs(debet-kredit),0,'f',2) );
    } else if( qAbs(debet) > 1e-5) {
        ui->varoTeksti->setText( tr("Summa %L1 €").arg(debet,0,'f',2) );
    }

    // Nappien enablointi
    // Täällä pitäisi olla jossain myös oikeuksien tarkastus ;)
    ui->tallennaButton->setEnabled( muokattu );
    ui->valmisNappi->setEnabled( (muokattu || tosite_->data(Tosite::TILA).toInt() < Tosite::KIRJANPIDOSSA  ) && !virheet);

    salliMuokkaus( !( virheet & Tosite::PVMALV || virheet & Tosite::PVMLUKITTU  ) || !tosite_->data(Tosite::ID).toInt() );
    if( muokattu )
        tallennettuWidget_->piiloon();

}

void KirjausWg::tallennettu(int /* id */, int tunniste, const QDate &pvm, const QString& sarja)
{
    if( ui->tositetyyppiCombo->currentData(TositeTyyppiModel::KoodiRooli) == TositeTyyppi::TILIOTE)
        ui->tositetyyppiCombo->setCurrentIndex(0);

    tallennettuWidget_->nayta(tunniste, pvm, sarja);

    tallennettuWidget_->move( width() / 2 - tallennettuWidget_->width() / 2,
                             height() - tallennettuWidget_->height() );
    tyhjenna();


}

void KirjausWg::tallennusEpaonnistui(int virhe)
{
    QMessageBox::critical(this, tr("Tallennus epäonnistui"), tr("Tositteen tallentaminen epäonnistui (Virhe %1)").arg(virhe));
}

void KirjausWg::tuonti(QVariant *data)
{
    QVariantMap map = data->toMap();
    if( map.contains("tyyppi"))
        ui->tositetyyppiCombo->setCurrentIndex( ui->tositetyyppiCombo->findData( map.value("tyyppi") ) );
    ui->tositePvmEdit->setDate( map.value("tositepvm").toDate() );
    if( apuri_)
        apuri_->tuo(map);
}

void KirjausWg::siirryTositteeseen()
{
    int id = SiirryDlg::tositeId( ui->tositePvmEdit->date(), QString() );

    if( id )
    {
        if( ui->tallennaButton->isEnabled() )
        {
            if( QMessageBox::question(this, tr("Kitupiikki"), tr("Nykyistä kirjausta on muokattu. Siirrytkö toiseen tositteeseen tallentamatta tekemiäsi muutoksia?")) != QMessageBox::Yes)
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
    else if( watched == ui->viennitView->viewport() )
    {
        // Merkkaus eli täggäys
        // Kohdennus-sarakkeessa hiiren oikealla napilla valikko, josta voi valita tägit
        if( event->type() == QEvent::MouseButtonPress)
        {
            QMouseEvent *mouseEvent = static_cast<QMouseEvent*>(event);
            if( mouseEvent->button() == Qt::RightButton)
            {
                QModelIndex index = ui->viennitView->indexAt( mouseEvent->pos() );
                if( index.column() == TositeViennit::KOHDENNUS && index.data(TositeViennit::PvmRooli).toDate().isValid() )
                {

                    tosite()->viennit()->setData(index, KohdennusProxyModel::tagiValikko( index.data(TositeViennit::PvmRooli).toDate(),
                                                                                            index.data(TositeViennit::TagiIdListaRooli).toList()) ,
                                                   TositeViennit::TagiIdListaRooli);
                    return false;
                }
            }
        }
    }

    if( watched == ui->viennitView && event->type() == QEvent::KeyPress)
    {
        QKeyEvent *keyEvent = static_cast<QKeyEvent*>(event);
        if( ( keyEvent->key() == Qt::Key_Enter ||
            keyEvent->key() == Qt::Key_Return ||
            keyEvent->key() == Qt::Key_Insert ||
            keyEvent->key() == Qt::Key_Tab) &&
                keyEvent->modifiers() == Qt::NoModifier )
        {

            // Insertillä suoraan uusi rivi
            if(  keyEvent->key() == Qt::Key_Insert )
            {
                lisaaRivi();
            }

            if( ui->viennitView->currentIndex().column() == TositeViennit::SELITE &&
                ui->viennitView->currentIndex().row() == tosite()->viennit()->rowCount() - 1 )
            {
                lisaaRivi();
                ui->viennitView->setCurrentIndex( tosite()->viennit()->index( tosite()->viennit()->rowCount(QModelIndex())-1, TositeViennit::TILI ) );
                return true;
            }

            else if( ui->viennitView->currentIndex().column() == TositeViennit::TILI )
            {
                ui->viennitView->setCurrentIndex( ui->viennitView->currentIndex().sibling( ui->viennitView->currentIndex().row(), TositeViennit::DEBET) );
                Tili tili = kp()->tilit()->tiliNumerolla( ui->viennitView->currentIndex().data(TositeViennit::TiliNumeroRooli).toInt() );                if( tili.onko(TiliLaji::TULO) || tili.onko(TiliLaji::VASTATTAVAA))
                    ui->viennitView->setCurrentIndex( ui->viennitView->currentIndex().sibling( ui->viennitView->currentIndex().row(),TositeViennit::KREDIT) );
                return true;
            }

            else if( ui->viennitView->currentIndex().column() == TositeViennit::DEBET)
            {
                ui->viennitView->setCurrentIndex( ui->viennitView->currentIndex().sibling( ui->viennitView->currentIndex().row(),TositeViennit::KREDIT) );
                qApp->processEvents();

                if( ui->viennitView->currentIndex().data(TositeViennit::DebetRooli).toInt()  )
                {
                    ui->viennitView->setCurrentIndex( ui->viennitView->currentIndex().sibling( ui->viennitView->currentIndex().row(),TositeViennit::KOHDENNUS) );

                }
                return true;
            }
            else if( ui->viennitView->currentIndex().column() < TositeViennit::ALV)
            {
                // Enterillä pääsee suoraan seuraavalle riville
                ui->viennitView->setCurrentIndex( ui->viennitView->currentIndex().sibling( ui->viennitView->currentIndex().row(), ui->viennitView->currentIndex().column()+1)  );
                return true;
            }


        }
    }


    return QWidget::eventFilter(watched, event);
}

void KirjausWg::paivitaLiiteNapit()
{
    bool liitteita = tosite()->liitteet()->rowCount(QModelIndex());

    ui->poistaLiiteNappi->setEnabled(liitteita);
    ui->avaaNappi->setEnabled(liitteita);

    if( liitteita )
        ui->tabWidget->setTabIcon( ui->tabWidget->indexOf(liitteetTab_) , QIcon(":/pic/liite-aktiivinen.png"));
    else
        ui->tabWidget->setTabIcon( ui->tabWidget->indexOf(liitteetTab_), QIcon(":/pic/liite"));
}


void KirjausWg::lataaTosite(int id)
{
    tosite_->lataa(id);
    return;

}

void KirjausWg::paivitaKommenttiMerkki()
{
    int kommenttiIndeksi = ui->tabWidget->indexOf(kommentitTab_);

    if( ui->kommentitEdit->document()->toPlainText().isEmpty())
    {
        ui->tabWidget->setTabIcon(kommenttiIndeksi, QIcon(":/pic/kommentti-harmaa.png"));
    }
    else
    {
        ui->tabWidget->setTabIcon(kommenttiIndeksi, QIcon(":/pic/kommentti.png"));
    }

    tosite_->setData(Tosite::KOMMENTIT, ui->kommentitEdit->toPlainText());

}

void KirjausWg::lisaaLiite(const QString& polku)
{
    if( !polku.isEmpty())
    {
        // Pyritään ensin tuomaan
        // PDF-tiedosto tuodaan kuitenkin vain tyhjälle tositteelle
        // Tämä siksi, että pdf-tiliote voidaan tuoda csv-tilitietojen tositteeksi
        if( !(polku.endsWith(".pdf",Qt::CaseInsensitive)
             && tosite()->liitteet()->rowCount() ) &&  !VanhaTuonti::tuo(polku, this))
            return;

        QFileInfo info(polku);
        tosite()->liitteet()->lisaaTiedosto(polku);
        // Valitsee lisätyn liitteen
        ui->liiteView->setCurrentIndex( tosite()->liitteet()->index( tosite()->liitteet()->rowCount() - 1 ) );
        paivitaLiiteNapit();
    }

}

void KirjausWg::lisaaLiite()
{
    lisaaLiite(QFileDialog::getOpenFileName(this, tr("Lisää liite"),QString(),tr("Pdf-tiedosto (*.pdf);;Kuvat (*.png *.jpg);;CSV-tiedosto (*.csv);;Kaikki tiedostot (*.*)")));
}

void KirjausWg::lisaaLiiteDatasta(const QByteArray &data, const QString &nimi)
{
      tosite()->liitteet()->lisaa(data, nimi);
      ui->liiteView->setCurrentIndex( tosite()->liitteet()->index( tosite()->liitteet()->rowCount() - 1 ) );
     paivitaLiiteNapit();

}


void KirjausWg::tiedotModelista()
{

    int tyyppi = tosite_->data(Tosite::TYYPPI).toInt();
    bool lisattavatyyppi = kp()->tositeTyypit()->onkolisattavissa(tyyppi);

    if( lisattavatyyppi )
        ui->tositetyyppiCombo->setCurrentIndex( ui->tositetyyppiCombo->findData( tosite_->data(Tosite::TYYPPI).toInt(), TositeTyyppiModel::KoodiRooli ) );
    else
        ui->tositetyyppiLabel->setText( kp()->tositeTyypit()->nimi(tyyppi) );
    ui->tositetyyppiCombo->setVisible( lisattavatyyppi );
    ui->tositetyyppiLabel->setVisible( !lisattavatyyppi );
    vaihdaTositeTyyppi();

    QDate tositepvm = tosite_->data(Tosite::PVM).toDate();

    ui->tositePvmEdit->setDate( tositepvm );
    ui->otsikkoEdit->setText( tosite_->data(Tosite::OTSIKKO).toString() );
    ui->kommentitEdit->setPlainText( tosite_->data(Tosite::KOMMENTIT).toString());

    ui->sarjaEdit->setText( tosite_->data(Tosite::SARJA).toString() );

    int tunniste = tosite_->data(Tosite::TUNNISTE).toInt();
    ui->sarjaLabel->setVisible( kp()->asetus("sarjaan").toInt() && !tunniste );

    if( tunniste ) {
        ui->tunnisteLabel->setVisible(true);
        ui->vuosiLabel->setVisible(true);
        ui->edellinenButton->setVisible(true);
        ui->seuraavaButton->setVisible(true);
        ui->tallennaButton->setVisible(false);

        ui->tunnisteLabel->setText( QString::number( tunniste ) );
        ui->vuosiLabel->setText( kp()->tilikaudet()->tilikausiPaivalle(tositepvm).pitkakausitunnus() );
    } else {
        ui->edellinenButton->setVisible(false);
        ui->tunnisteLabel->setVisible(false);
        ui->seuraavaButton->setVisible(false);
        ui->vuosiLabel->setVisible(false);
    }

}

void KirjausWg::salliMuokkaus(bool sallitaanko)
{
    ui->tositePvmEdit->setEnabled(sallitaanko);
    ui->tositetyyppiCombo->setEnabled(sallitaanko);
    ui->kommentitEdit->setEnabled(sallitaanko);
    ui->otsikkoEdit->setEnabled(sallitaanko);
    ui->lisaaliiteNappi->setEnabled(sallitaanko);
    ui->poistaLiiteNappi->setEnabled(sallitaanko);

    if(sallitaanko)
        ui->tositePvmEdit->setDateRange( kp()->tilitpaatetty().addDays(1), kp()->tilikaudet()->kirjanpitoLoppuu() );
    else
        ui->tositePvmEdit->setDateRange( kp()->tilikaudet()->kirjanpitoAlkaa(), kp()->tilikaudet()->kirjanpitoLoppuu() );

    tosite_->viennit()->asetaMuokattavissa( sallitaanko && !apuri_ );
    ui->lisaaRiviNappi->setVisible( !apuri_);
    ui->poistariviNappi->setVisible( !apuri_);
}

void KirjausWg::vaihdaTositeTyyppi()
{
    paivitaSarja();
    int tyyppiKoodi = ui->tositetyyppiCombo->currentData(TositeTyyppiModel::KoodiRooli).toInt() ;

    // Tässä voisi laittaa muutenkin apurit paikalleen
    if( apuri_ )
    {
        ui->tabWidget->removeTab( ui->tabWidget->indexOf( apuri_) );
        apuri_->deleteLater();
    }
    apuri_ = nullptr;


    // Liitetiedoilla ei ole vientejä
    ui->tabWidget->setTabEnabled( ui->tabWidget->indexOf(viennitTab_) , tyyppiKoodi != TositeTyyppi::LIITETIETO);

    // Varasto ei toistaiseksi käytössä
    // ui->tabWidget->setTabEnabled( ui->tabWidget->indexOf(varastoTab_), false);

    if( tyyppiKoodi == TositeTyyppi::TULO || tyyppiKoodi == TositeTyyppi::MENO || tyyppiKoodi == TositeTyyppi::KULULASKU )
    {
        apuri_ = new TuloMenoApuri(this, tosite_);
    } else if( tyyppiKoodi == TositeTyyppi::SIIRTO) {
        apuri_ = new SiirtoApuri(this, tosite_);
    } else if( tyyppiKoodi == TositeTyyppi::TILIOTE ) {
        apuri_ = new TilioteApuri(this, tosite_);
    } else if( tyyppiKoodi == TositeTyyppi::PALKKA) {
        apuri_ = new PalkkaApuri(this, tosite_);
    }

    tosite_->setData(Tosite::TYYPPI, tyyppiKoodi);

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

    if( ui->otsikkoEdit->text().startsWith("Tiliote") && tyyppiKoodi != TositeTyyppi::TILIOTE)
        ui->otsikkoEdit->clear();    
}

void KirjausWg::paivitaSarja(bool kateinen)
{
    if( kp()->asetukset()->asetus("sarjaan").toInt())
        ui->sarjaEdit->setText( kp()->tositeTyypit()->sarja( ui->tositetyyppiCombo->currentData(TositeTyyppiModel::KoodiRooli).toInt() , kateinen ) ) ;
}

void KirjausWg::liiteValinta(const QModelIndex &valittu)
{
    if( !valittu.isValid())
    {
        ui->poistaLiiteNappi->setDisabled(true);
        emit liiteValittu( QByteArray());
    }
    else
    {
        ui->poistaLiiteNappi->setEnabled(true);
        tosite_->liitteet()->nayta( valittu.row() );

        // emit liiteValittu( valittu.data(LiiteModel::PdfRooli).toByteArray() );
    }
}

void KirjausWg::pvmVaihtuu()
{

    QDate paiva = ui->tositePvmEdit->date();
    QDate vanhaPaiva = tosite_->data(Tosite::PVM).toDate();

    tosite_->setData(Tosite::PVM, paiva);

    if( kp()->tilikaudet()->tilikausiPaivalle(paiva).alkaa() != kp()->tilikaudet()->tilikausiPaivalle(vanhaPaiva).alkaa())
    {
        // Siirrytty toiselle tilikaudelle, vaihdetaan numerointia
        // Tallennettaessa uusi numero palvelimelta
        tosite_->setData( Tosite::TUNNISTE, QVariant() );
    }

}

void KirjausWg::poistaLiite()
{
    /*
    if( ui->liiteView->currentIndex().isValid() && model_->muokkausSallittu() )
    {
        if( QMessageBox::question(this, tr("Poista liite"),
                                  tr("Poistetaanko liite %1. Poistettua liitettä ei voi palauttaa!").arg( ui->liiteView->currentIndex().data(LiiteModel::OtsikkoRooli).toString()),
                                  QMessageBox::Yes, QMessageBox::Cancel) == QMessageBox::Yes )
        {
            model_->liiteModel()->poistaLiite( ui->liiteView->currentIndex().row() );
        }
    }
    ui->poistaLiiteNappi->setEnabled( model()->liiteModel()->rowCount(QModelIndex()) );
    */
}

