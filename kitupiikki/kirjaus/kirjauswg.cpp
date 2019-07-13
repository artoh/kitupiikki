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
#include "kirjausapuridialog.h"
#include "kohdennusdelegaatti.h"
#include "taseeravalintadialogi.h"

#include "verodialogi.h"

#include "siirrydlg.h"

#include "db/kirjanpito.h"
#include "laskutus/laskunmaksudialogi.h"

#include "tuonti/tuonti.h"
#include "apurivinkki.h"
#include "ui_numerosiirto.h"
#include "naytin/naytinikkuna.h"
#include "ui_kopioitosite.h"

#include "edellinenseuraavatieto.h"
#include "verotarkastaja.h"
#include "db/tositetyyppimodel.h"

#include "apuri/tulomenoapuri.h"
#include "apuri/siirtoapuri.h"
#include "apuri/tilioteapuri.h"
#include "model/tosite.h"
#include "model/tositeliitteet.h"
#include "model/tositeviennit.h"
#include "model/tositeloki.h"
#include "tallennettuwidget.h"


KirjausWg::KirjausWg(TositeModel *tositeModel, QWidget *parent)
    : QWidget(parent), model_(tositeModel), laskuDlg_(nullptr), apurivinkki_(nullptr),
      taydennysSql_( new QSqlQueryModel ), apuri_(nullptr),
      tallennettuWidget_( new TallennettuWidget(this) )
{
    ui = new Ui::KirjausWg();
    ui->setupUi(this);

    connect( model_->vientiModel(), SIGNAL(muuttunut()), this, SLOT(naytaSummat()));

    // Tämä pitää säilyttää, jotta saadaan päivämäärä paikalleen
    ui->viennitView->setItemDelegateForColumn( VientiModel::PVM, new PvmDelegaatti(ui->tositePvmEdit));



    connect( ui->lisaaRiviNappi, SIGNAL(clicked(bool)), this, SLOT(lisaaRivi()));
    connect( ui->poistariviNappi, SIGNAL(clicked(bool)), this, SLOT(poistaRivi()));
    connect( ui->tallennaButton, SIGNAL(clicked(bool)), this, SLOT(tallenna()));
    connect( ui->valmisNappi, &QPushButton::clicked, this, &KirjausWg::valmis);
    connect( ui->hylkaaNappi, SIGNAL(clicked(bool)), this, SLOT(hylkaa()));
    connect( ui->kommentitEdit, SIGNAL(textChanged()), this, SLOT(paivitaKommenttiMerkki()));

    tyyppiProxy_ = new QSortFilterProxyModel(this);
    tyyppiProxy_->setSourceModel( kp()->tositeTyypit() );
    tyyppiProxy_->setFilterRole( TositeTyyppiModel::KoodiRooli);
    tyyppiProxy_->setFilterRegularExpression("^[^9]");

    qDebug() << "tyyppiProxy " << tyyppiProxy_->rowCount(QModelIndex()) << " -- " << kp()->tositeTyypit()->rowCount(QModelIndex());


    ui->tositetyyppiCombo->setModel( tyyppiProxy_ );


    // Kun tositteen päivää vaihdetaan, vaihtuu myös tiliotepäivät.
    // Siksi tosipäivä ladattava aina ennen tiliotepäiviä!
    connect( ui->tositePvmEdit, SIGNAL(editingFinished()), this, SLOT(pvmVaihtuu()));

    connect( ui->tositetyyppiCombo, SIGNAL(currentIndexChanged(int)), this, SLOT(vaihdaTositeTyyppi()));
    connect( ui->viennitView, SIGNAL(activated(QModelIndex)), this, SLOT( vientivwAktivoitu(QModelIndex)));
    connect( ui->viennitView->selectionModel(), SIGNAL(currentRowChanged(QModelIndex,QModelIndex)), this, SLOT(vientiValittu()));


    ui->tiliotetiliCombo->suodataTyypilla("ARP");



    connect( ui->liiteView->selectionModel(), SIGNAL(currentChanged(QModelIndex,QModelIndex)),
             this, SLOT(liiteValinta(QModelIndex)));
    connect( ui->lisaaliiteNappi, SIGNAL(clicked(bool)), this, SLOT(lisaaLiite()));
    connect( ui->avaaNappi, &QPushButton::clicked, this, &KirjausWg::avaaLiite);
    connect( ui->tulostaLiiteNappi, &QPushButton::clicked, this, &KirjausWg::tulostaLiite);
    connect( ui->poistaLiiteNappi, SIGNAL(clicked(bool)), this, SLOT(poistaLiite()));

    connect( ui->tiliotetiliCombo, SIGNAL(activated(int)), this, SLOT(tiedotModeliin()));

    connect( model(), SIGNAL(tositettaMuokattu(bool)), this, SLOT(paivitaTallennaPoistaNapit()));

    // Lisätoimintojen valikko
    QMenu *valikko = new QMenu(this);
    valikko->addAction(QIcon(":/pic/etsi.png"), tr("Siirry tositteeseen\tCtrl+G"), this, SLOT(siirryTositteeseen()));
    valikko->addAction(QIcon(":/pic/tulosta.png"), tr("Tulosta tosite\tCtrl+P"), this, SLOT(tulostaTosite()), QKeySequence("Ctrl+P"));
    uudeksiAktio_ = valikko->addAction(QIcon(":/pic/kopioi.png"), tr("Kopioi uuden pohjaksi\tCtrl+T"), this, SLOT(uusiPohjalta()), QKeySequence("Ctrl+T"));
    poistaAktio_ = valikko->addAction(QIcon(":/pic/roskis.png"),tr("Poista tosite"),this, SLOT(poistaTosite()));
    tyhjennaViennitAktio_ = valikko->addAction(QIcon(":/pic/edit-clear.png"),tr("Tyhjennä viennit"), model_->vientiModel(), &VientiModel::tyhjaa);

    ui->valikkoNappi->setMenu( valikko );


    // Enterillä päiväyksestä eteenpäin
    ui->tositePvmEdit->installEventFilter(this);
    ui->otsikkoEdit->installEventFilter(this);
    ui->tositetyyppiCombo->installEventFilter(this);

    // Tagivalikko
    ui->viennitView->viewport()->installEventFilter(this);

    // ui->viennitView->installEventFilter(this);
    ui->viennitView->setFocusPolicy(Qt::StrongFocus);

    QCompleter *otsikonTaydentaja = new QCompleter(taydennysSql_, this);
    otsikonTaydentaja->setModelSorting(QCompleter::CaseSensitivelySortedModel);
    ui->otsikkoEdit->setCompleter(otsikonTaydentaja);
    connect( ui->otsikkoEdit, SIGNAL(textChanged(QString)), this, SLOT(paivitaOtsikonTaydennys(QString)));

    edellinenSeuraava_ = new EdellinenSeuraavaTieto( model(), this );
    connect( edellinenSeuraava_, &EdellinenSeuraavaTieto::edellinenOlemassa, ui->edellinenButton, &QPushButton::setEnabled );
    connect( edellinenSeuraava_, &EdellinenSeuraavaTieto::seuraavaOlemassa, ui->seuraavaButton, &QPushButton::setEnabled );

    connect( ui->edellinenButton, &QPushButton::clicked, [this] () { this->lataaTosite(this->edellinenSeuraava_->edellinenId()); });
    connect( ui->seuraavaButton, &QPushButton::clicked, [this] () { this -> lataaTosite(this->edellinenSeuraava_->seuraavaId()); });

    connect( model(), &TositeModel::modelReset, this, &KirjausWg::tiedotModelista);

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



    vaihdaTositeTyyppi();
}

KirjausWg::~KirjausWg()
{
    // Tallennetaan ruudukon sarakkeiden leveydet
    QStringList leveysLista;
    for(int i=0; i<ui->viennitView->model()->columnCount(); i++)
        leveysLista.append( QString::number( ui->viennitView->horizontalHeader()->sectionSize(i) ) );

    kp()->settings()->setValue("KirjausWgRuudukko", leveysLista);

    delete ui;
    delete model_;
}

void KirjausWg::lisaaRivi()
{   
    if( apurivinkki_ )
        apurivinkki_->hide();

    // Lisätään valinnan jälkeen
    QModelIndex indeksi = model_->vientiModel()->lisaaVienti(ui->viennitView->currentIndex().isValid() ? ui->viennitView->currentIndex().row() + 1 : -1);

    ui->viennitView->setFocus();

    ui->viennitView->setCurrentIndex( indeksi.sibling( indeksi.row(), VientiModel::TILI )  );

}

void KirjausWg::poistaRivi()
{
    QModelIndex nykyinen = ui->viennitView->currentIndex();
    if( nykyinen.isValid() && nykyinen.sibling(nykyinen.row(), VientiModel::SELITE).flags() & Qt::ItemIsEditable)
    {
        model_->vientiModel()->poistaRivi( nykyinen.row());
    }
}

void KirjausWg::tyhjenna()
{
    tosite_->nollaa( ui->tositePvmEdit->date(), ui->tositetyyppiCombo->currentData(TositeTyyppiModel::KoodiRooli).toInt() );
    tiedotModelista();
    ui->tallennaButton->setVisible(true);
    ui->tositePvmEdit->setFocus();
    ui->tabWidget->setCurrentIndex(0);
    ui->tositePvmEdit->setFocus();
}

void KirjausWg::tallenna()
{

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
        if( model()->poista())
        {
            tyhjenna();
            emit tositeKasitelty();
        }
        else
            QMessageBox::critical(this, tr("Tietokantavirhe"),
                                  tr("Tietokantavirhe tositetta poistettaessa\n\n%1").arg( kp()->viimeVirhe() ));
    }
}

void KirjausWg::vientiValittu()
{
    QModelIndex index = ui->viennitView->selectionModel()->currentIndex();
    QDate vientiPvm = index.data(VientiModel::PvmRooli).toDate();
    ui->poistariviNappi->setEnabled( index.isValid() &&  ( vientiPvm > kp()->tilitpaatetty() || vientiPvm.isNull() ));
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
        model_->uusiPohjalta( kui.pvmEdit->date(), kui.otsikkoEdit->text() );
        tiedotModelista();
        paivitaTallennaPoistaNapit();
        emit liiteValittu(QByteArray());
    }

}

void KirjausWg::vientivwAktivoitu(QModelIndex indeksi)
{
    // Tehdään alv-kirjaus
    if( model()->muokkausSallittu() )
    {

        if(indeksi.column() == VientiModel::ALV )
        {
            VeroDialogi verodlg(this);
            if( verodlg.nayta( indeksi.data(VientiModel::AlvKoodiRooli).toInt(), indeksi.data(VientiModel::AlvProsenttiRooli).toInt() ))
            {
                model_->vientiModel()->setData(indeksi, verodlg.alvKoodi() , VientiModel::AlvKoodiRooli);
                model_->vientiModel()->setData(indeksi, verodlg.alvProsentti() , VientiModel::AlvProsenttiRooli);
            }
        }
        else if(indeksi.column() == VientiModel::KOHDENNUS && indeksi.data(VientiModel::TaseErittelyssaRooli).toBool())
        {
            TaseEraValintaDialogi dlg(this);
            dlg.nayta( model_->vientiModel(), indeksi );
        }
    }
}

void KirjausWg::kirjaaLaskunmaksu()
{
    if( !laskuDlg_ )
        laskuDlg_ = new LaskunMaksuDialogi(this);

    laskuDlg_->show();
    // connect( laskuDlg_, SIGNAL(finished(int)), this, SLOT(tiedotModelista()));

}

void KirjausWg::paivitaTallennaPoistaNapit()
{
    poistaAktio_->setEnabled( model()->muokattu() && model_->id() > -1 && model()->muokkausSallittu());
    tyhjennaViennitAktio_->setEnabled(model()->muokattu() && model_->id() > -1 && model()->muokkausSallittu() );
    uudeksiAktio_->setEnabled( !model()->muokattu() );


}


void KirjausWg::numeroSiirto()
{
    QDialog dlg;
    Ui::NumeroSiirtoDialog dui;
    dui.setupUi(&dlg);

    Tilikausi kausi = kp()->tilikaudet()->tilikausiPaivalle( ui->tositePvmEdit->date());

    dui.tilikausiLabel->setText( kausi.kausivaliTekstina() );
    dui.lajiLabel->setText( ui->tositetyyppiCombo->currentText() );
    dui.alkuSpin->setMaximum( model_->seuraavaTunnistenumero() );

    if( dlg.exec() == QDialog::Accepted )
    {
        // Siirretään tunnistenumeroita eteenpäin

        QString kasky = QString("UPDATE tosite SET tunniste = tunniste + %1 WHERE laji = %2 AND tunniste >= %3 AND pvm BETWEEN '%4' AND '%5'")
                .arg( dui.lisaaSpin->value() )
                .arg( ui->tositetyyppiCombo->currentData(TositelajiModel::IdRooli).toInt() )
                .arg( dui.alkuSpin->value())
                .arg( kausi.alkaa().toString(Qt::ISODate) )
                .arg( kausi.paattyy().toString(Qt::ISODate));

        QSqlQuery kysely(kasky);

        paivitaTunnisteVari();
    }

}

void KirjausWg::tulostaTosite()
{
    QPrintDialog printDialog( kp()->printer(), this);
    if( printDialog.exec() )
    {
        QPainter painter( kp()->printer() );
        model()->tuloste().tulosta( kp()->printer(), &painter );
        painter.end();
    }
}

void KirjausWg::naytaSelvitys()
{
    NaytinIkkuna *naytin = new NaytinIkkuna();
    naytin->naytaRaportti( model()->selvittelyTuloste() );
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
    ui->valmisNappi->setEnabled(muokattu && !virheet);

    salliMuokkaus( !( virheet & Tosite::PVMALV || virheet & Tosite::PVMLUKITTU  ) || !tosite_->data(Tosite::ID).toInt() );
    if( muokattu )
        tallennettuWidget_->piiloon();

}

void KirjausWg::tallennettu(int /* id */, int tunniste, const QDate &pvm)
{

    if( tunniste ) {
            tallennettuWidget_->nayta(tunniste, pvm);

//            tallennettuWidget_->move( mapToGlobal( QPoint(width() / 2 - tallennettuWidget_->width() / 2,
//                                                   height() - tallennettuWidget_->height() ) )  );


            tallennettuWidget_->move( width() / 2 - tallennettuWidget_->width() / 2,
                                     height() - tallennettuWidget_->height() );
        tyhjenna();
    }
}

void KirjausWg::tallennusEpaonnistui(int virhe)
{
    QMessageBox::critical(this, tr("Tallennus epäonnistui"), tr("Tositteen tallentaminen epäonnistui (Virhe %1)").arg(virhe));
}

void KirjausWg::siirryTositteeseen()
{
    int id = SiirryDlg::tositeId( ui->tositePvmEdit->date(), ui->tositetyyppiCombo->currentData(TositelajiModel::TunnusRooli).toString()  );

    if( id )
    {
        if( model_->muokattu())
        {
            if( QMessageBox::question(this, tr("Kitupiikki"), tr("Nykyistä kirjausta on muokattu. Siirrytkö toiseen tositteeseen tallentamatta tekemiäsi muutoksia?")) != QMessageBox::Yes)
            {
                return;
            }
        }
        lataaTosite(id);
    }
}

void KirjausWg::paivitaOtsikonTaydennys(const QString &teksti)
{
    if( teksti.length() > 2 && !teksti.contains(QChar('\'')))
        taydennysSql_->setQuery(QString("SELECT otsikko FROM tosite WHERE otsikko LIKE '%1%' order by otsikko").arg(teksti));
    else
        taydennysSql_->clear();

    model()->asetaOtsikko(teksti);
    tosite_->setData(Tosite::OTSIKKO, teksti);
}

int KirjausWg::tiliotetiliId()
{
    return ui->tiliotetiliCombo->currentData(TiliModel::IdRooli).toInt();
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
            // Tositetyypistä pääsee tabulaattorilla uudelle riville
            else if( keyEvent->key() == Qt::Key_Tab && watched == ui->tositetyyppiCombo)
            {
                if( apuri_) {
                    apuri_->otaFokus();
                    return true;
                }

                if( !model()->vientiModel()->rowCount(QModelIndex()))
                    lisaaRivi();
                ui->viennitView->setFocus(Qt::TabFocusReason);
                ui->viennitView->setCurrentIndex( ui->viennitView->model()->index(0,VientiModel::TILI));
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
                if( index.column() == VientiModel::KOHDENNUS && index.data(VientiModel::PvmRooli).toDate().isValid() )
                {

                    model_->vientiModel()->setData(index, KohdennusProxyModel::tagiValikko( index.data(VientiModel::PvmRooli).toDate(),
                                                                                            index.data(VientiModel::TagiIdListaRooli).toList()) ,
                                                   VientiModel::TagiIdListaRooli);
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
            if( ( keyEvent->key() == Qt::Key_Insert )
                    && ui->viennitView->currentIndex().row() == model()->vientiModel()->rowCount(QModelIndex()) - 1 )
            {
                lisaaRivi();
                ui->viennitView->setCurrentIndex( model()->vientiModel()->index( model()->vientiModel()->rowCount(QModelIndex())-1, VientiModel::TILI ) );
            }

            if( ui->viennitView->currentIndex().column() == VientiModel::SELITE &&
                ui->viennitView->currentIndex().row() == model()->vientiModel()->rowCount(QModelIndex()) - 1 )
            {
                lisaaRivi();
                ui->viennitView->setCurrentIndex( model()->vientiModel()->index( model()->vientiModel()->rowCount(QModelIndex())-1, VientiModel::TILI ) );
                return true;
            }

            else if( ui->viennitView->currentIndex().column() == VientiModel::TILI )
            {
                ui->viennitView->setCurrentIndex( ui->viennitView->currentIndex().sibling( ui->viennitView->currentIndex().row(), VientiModel::DEBET) );
                Tili tili = kp()->tilit()->tiliIdllaVanha( ui->viennitView->currentIndex().data(VientiModel::TiliIdRooli).toInt() );
                if( tili.onko(TiliLaji::TULO) || tili.onko(TiliLaji::VASTATTAVAA))
                    ui->viennitView->setCurrentIndex( ui->viennitView->currentIndex().sibling( ui->viennitView->currentIndex().row(),VientiModel::KREDIT) );
                return true;
            }

            else if( ui->viennitView->currentIndex().column() == VientiModel::DEBET)
            {
                ui->viennitView->setCurrentIndex( ui->viennitView->currentIndex().sibling( ui->viennitView->currentIndex().row(),VientiModel::KREDIT) );
                qApp->processEvents();

                if( ui->viennitView->currentIndex().data(VientiModel::DebetRooli).toInt()  )
                {
                    ui->viennitView->setCurrentIndex( ui->viennitView->currentIndex().sibling( ui->viennitView->currentIndex().row(),VientiModel::KOHDENNUS) );

                }
                return true;
            }
            else if( ui->viennitView->currentIndex().column() < VientiModel::ALV)
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
    bool liitteita = model()->liiteModel()->rowCount(QModelIndex());

    ui->poistaLiiteNappi->setEnabled(liitteita);
    ui->avaaNappi->setEnabled(liitteita);

    if( liitteita )
        ui->tabWidget->setTabIcon(LIITTEET, QIcon(":/pic/liite-aktiivinen.png"));
    else
        ui->tabWidget->setTabIcon(LIITTEET, QIcon(":/pic/liite"));
}


void KirjausWg::lataaTosite(int id)
{
    tosite_->lataa(id);
    return;

}

void KirjausWg::paivitaKommenttiMerkki()
{
    int kommenttiIndeksi = apuri_ ? KOMMENTIT + 1 : KOMMENTIT;

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

void KirjausWg::paivitaTunnisteVari()
{
    paivitaTallennaPoistaNapit();
}

void KirjausWg::lisaaLiite(const QString& polku)
{
    if( !polku.isEmpty())
    {
        // Pyritään ensin tuomaan
        // PDF-tiedosto tuodaan kuitenkin vain tyhjälle tositteelle
        // Tämä siksi, että pdf-tiliote voidaan tuoda csv-tilitietojen tositteeksi
        if( !(polku.endsWith(".pdf",Qt::CaseInsensitive)
             && model()->vientiModel()->rowCount(QModelIndex()) ) &&  !Tuonti::tuo(polku, this))
            return;

        QFileInfo info(polku);
        model_->liiteModel()->lisaaTiedosto(polku, info.fileName());
        // Valitsee lisätyn liitteen
        ui->liiteView->setCurrentIndex( model_->liiteModel()->index( model_->liiteModel()->rowCount(QModelIndex()) - 1 ) );
        paivitaLiiteNapit();
    }

}

void KirjausWg::lisaaLiite()
{
    lisaaLiite(QFileDialog::getOpenFileName(this, tr("Lisää liite"),QString(),tr("Pdf-tiedosto (*.pdf);;Kuvat (*.png *.jpg);;CSV-tiedosto (*.csv);;Kaikki tiedostot (*.*)")));
}

void KirjausWg::lisaaLiiteDatasta(const QByteArray &data, const QString &nimi)
{
    model_->liiteModel()->lisaaLiite( data, nimi );
    // Valitsee lisätyn liitteen
    ui->liiteView->setCurrentIndex( model_->liiteModel()->index( model_->liiteModel()->rowCount(QModelIndex()) - 1 ) );
    paivitaLiiteNapit();

}


void KirjausWg::tiedotModelista()
{

    QDate tositepvm = tosite_->data(Tosite::PVM).toDate();

    ui->tositePvmEdit->setDate( tositepvm );
    ui->otsikkoEdit->setText( tosite_->data(Tosite::OTSIKKO).toString() );
    ui->kommentitEdit->setPlainText( tosite_->data(Tosite::KOMMENTIT).toString());

    int tunniste = tosite_->data(Tosite::TUNNISTE).toInt();

    if( tunniste ) {
        ui->tunnisteLabel->setVisible(true);
        ui->edellinenButton->setVisible(true);
        ui->seuraavaButton->setVisible(true);
        ui->tallennaButton->setVisible(false);

        ui->tunnisteLabel->setText( QString("%1 / %2")
                                    .arg( tunniste )
                                    .arg( kp()->tilikaudet()->tilikausiPaivalle(tositepvm).kausitunnus() ));
    } else {
        ui->edellinenButton->setVisible(false);
        ui->tunnisteLabel->setVisible(false);
        ui->seuraavaButton->setVisible(false);
    }

    ui->tositetyyppiCombo->setCurrentIndex( ui->tositetyyppiCombo->findData( tosite_->data(Tosite::TYYPPI).toInt(), TositeTyyppiModel::KoodiRooli ) );

    if( apuri_ )
        apuri_->reset();

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
    model_->asetaTositelaji( ui->tositetyyppiCombo->currentData(TositelajiModel::IdRooli).toInt() );

    int tyyppiKoodi = ui->tositetyyppiCombo->currentData(TositeTyyppiModel::KoodiRooli).toInt() ;

    // Tässä voisi laittaa muutenkin apurit paikalleen
    if( apuri_ )
    {
        ui->tabWidget->removeTab( ui->tabWidget->indexOf( apuri_) );
        apuri_->deleteLater();
    }
    apuri_ = nullptr;
    ui->tabWidget->setTabEnabled(0, tyyppiKoodi != TositeTyyppi::LIITETIETO);
    ui->tiliotetiliCombo->setVisible( tyyppiKoodi == TositeTyyppi::TILIOTE );

    if( tyyppiKoodi == TositeTyyppi::TULO || tyyppiKoodi == TositeTyyppi::MENO)
    {
        apuri_ = new TuloMenoApuri(this, tosite_);
    } else if( tyyppiKoodi == TositeTyyppi::SIIRTO) {
        apuri_ = new SiirtoApuri(this, tosite_);
    } else if( tyyppiKoodi == TositeTyyppi::TILIOTE ) {
        apuri_ = new TilioteApuri(this, tosite_);
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
        emit liiteValittu( valittu.data(LiiteModel::PdfRooli).toByteArray() );
    }
}

void KirjausWg::pvmVaihtuu()
{
    if( !model_->muokkausSallittu() )
        return;

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
}

