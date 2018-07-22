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
#include "tools/pdfikkuna.h"
#include "ui_kopioitosite.h"



KirjausWg::KirjausWg(TositeModel *tositeModel, QWidget *parent)
    : QWidget(parent), model_(tositeModel), laskuDlg_(nullptr), apurivinkki_(nullptr),
      taydennysSql_( new QSqlQueryModel )
{
    ui = new Ui::KirjausWg();
    ui->setupUi(this);

    ui->viennitView->setModel( model_->vientiModel() );

    connect( model_->vientiModel(), SIGNAL(muuttunut()), this, SLOT(naytaSummat()));

    ui->viennitView->setItemDelegateForColumn( VientiModel::PVM, new PvmDelegaatti(ui->tositePvmEdit));
    ui->viennitView->setItemDelegateForColumn( VientiModel::TILI, new TiliDelegaatti( ) );
    ui->viennitView->setItemDelegateForColumn( VientiModel::DEBET, new EuroDelegaatti);
    ui->viennitView->setItemDelegateForColumn( VientiModel::KREDIT, new EuroDelegaatti);
    ui->viennitView->setItemDelegateForColumn( VientiModel::KOHDENNUS, new KohdennusDelegaatti);

    ui->viennitView->horizontalHeader()->setStretchLastSection(true);

    ui->tunnisteEdit->setValidator( new QIntValidator(1,99999999) );

    connect( ui->lisaaRiviNappi, SIGNAL(clicked(bool)), this, SLOT(lisaaRivi()));
    connect( ui->poistariviNappi, SIGNAL(clicked(bool)), this, SLOT(poistaRivi()));
    connect( ui->tallennaButton, SIGNAL(clicked(bool)), this, SLOT(tallenna()));
    connect( ui->hylkaaNappi, SIGNAL(clicked(bool)), this, SLOT(hylkaa()));
    connect( ui->kommentitEdit, SIGNAL(textChanged()), this, SLOT(paivitaKommenttiMerkki()));
    connect( ui->apuriNappi, SIGNAL(clicked(bool)), this, SLOT(kirjausApuri()));
    connect( ui->laskuNappi, SIGNAL(clicked(bool)), this, SLOT(kirjaaLaskunmaksu()));
    connect( ui->siiraNumerotBtn, SIGNAL(clicked(bool)), this, SLOT(numeroSiirto()));

    ui->tositetyyppiCombo->setModel( Kirjanpito::db()->tositelajit());
    ui->tositetyyppiCombo->setModelColumn( TositelajiModel::NIMI);    

    // Kun tositteen päivää vaihdetaan, vaihtuu myös tiliotepäivät.
    // Siksi tosipäivä ladattava aina ennen tiliotepäiviä!
    connect( ui->tositePvmEdit, SIGNAL(editingFinished()), this, SLOT(pvmVaihtuu()));

    connect( ui->tositetyyppiCombo, SIGNAL(currentIndexChanged(int)), this, SLOT(vaihdaTositeTyyppi()));
    connect( ui->tunnisteEdit, SIGNAL(textChanged(QString)), this, SLOT(paivitaTunnisteVari()));
    connect( ui->viennitView, SIGNAL(activated(QModelIndex)), this, SLOT( vientivwAktivoitu(QModelIndex)));
    connect( ui->viennitView->selectionModel(), SIGNAL(currentRowChanged(QModelIndex,QModelIndex)), this, SLOT(vientiValittu()));


    // Tiliotteen tilivalintaan hyväksytään vain rahoitustilit
    QSortFilterProxyModel *proxy = new QSortFilterProxyModel(this);
    proxy->setSourceModel( kp()->tilit());
    proxy->setFilterRole( TiliModel::TyyppiRooli);
    proxy->setFilterFixedString("AR");
    proxy->setSortRole(TiliModel::NroRooli);

    ui->tiliotetiliCombo->setModel( proxy );
    ui->tiliotetiliCombo->setModelColumn(TiliModel::NRONIMI);

    ui->liiteView->setModel( model_->liiteModel() );
    connect( ui->liiteView->selectionModel(), SIGNAL(currentChanged(QModelIndex,QModelIndex)),
             this, SLOT(liiteValinta(QModelIndex)));
    connect( ui->lisaaliiteNappi, SIGNAL(clicked(bool)), this, SLOT(lisaaLiite()));
    connect( ui->avaaNappi, SIGNAL(clicked(bool)), this, SLOT(naytaLiite()));
    connect( ui->poistaLiiteNappi, SIGNAL(clicked(bool)), this, SLOT(poistaLiite()));

    connect( ui->tiliotealkaenEdit, SIGNAL(editingFinished()), this, SLOT(tiedotModeliin()));
    connect( ui->tilioteloppuenEdit, SIGNAL(editingFinished()), this, SLOT(tiedotModeliin()));
    connect( ui->tilioteBox, SIGNAL(clicked(bool)), this, SLOT(tiedotModeliin()));
    connect( ui->tiliotetiliCombo, SIGNAL(activated(int)), this, SLOT(tiedotModeliin()));

    connect( model(), SIGNAL(tositettaMuokattu(bool)), this, SLOT(paivitaTallennaPoistaNapit()));

    // Lisätoimintojen valikko
    QMenu *valikko = new QMenu(this);
    valikko->addAction(QIcon(":/pic/etsi.png"), tr("Siirry tositteeseen\tCtrl+G"), this, SLOT(siirryTositteeseen()));
    valikko->addAction(QIcon(":/pic/tulosta.png"), tr("Tulosta tosite\tCtrl+P"), this, SLOT(tulostaTosite()), QKeySequence("Ctrl+P"));
    uudeksiAktio_ = valikko->addAction(QIcon(":/pic/kopioi.png"), tr("Kopioi uuden pohjaksi\tCtrl+T"), this, SLOT(uusiPohjalta()), QKeySequence("Ctrl+T"));
    poistaAktio_ = valikko->addAction(QIcon(":/pic/roskis.png"),tr("Poista tosite"),this, SLOT(poistaTosite()));
    ui->valikkoNappi->setMenu( valikko );


    // Enterillä päiväyksestä eteenpäin
    ui->tositePvmEdit->installEventFilter(this);
    ui->otsikkoEdit->installEventFilter(this);
    ui->tositetyyppiCombo->installEventFilter(this);

    // Tagivalikko
    ui->viennitView->viewport()->installEventFilter(this);

    ui->viennitView->installEventFilter(this);
    ui->viennitView->setFocusPolicy(Qt::StrongFocus);

    ui->tositePvmEdit->setCalendarPopup(true);

    QCompleter *otsikonTaydentaja = new QCompleter(taydennysSql_, this);
    otsikonTaydentaja->setModelSorting(QCompleter::CaseSensitivelySortedModel);
    ui->otsikkoEdit->setCompleter(otsikonTaydentaja);
    connect( ui->otsikkoEdit, SIGNAL(textChanged(QString)), this, SLOT(paivitaOtsikonTaydennys(QString)));
}

KirjausWg::~KirjausWg()
{
    delete ui;
    delete model_;
}

QDate KirjausWg::tositePvm() const
{
    return ui->tositePvmEdit->date();
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
    // Tunnisteen väri mustaksi
    ui->tunnisteEdit->setStyleSheet("color: black;");
    // Tyhjennetään ensin model
    model_->tyhjaa();
    // ja sitten päivitetään lomakkeen tiedot modelista
    tiedotModelista();
    // Ei voi tallentaa eikä poistaa kun ei ole mitään...
    ui->tallennaButton->setEnabled(false);
    poistaAktio_->setEnabled(false);

    ui->poistaLiiteNappi->setEnabled(false);
    pvmVaihtuu();
    // Verosarake näytetään vain, jos alv-toiminnot käytössä
    ui->viennitView->setColumnHidden( VientiModel::ALV, !kp()->asetukset()->onko("AlvVelvollinen") );
    // Tyhjennetään tositemodel
    emit liiteValittu(QByteArray());
    // Tyhjennetään laskudialogi
    if( laskuDlg_)
    {
        laskuDlg_->deleteLater();
        laskuDlg_ = nullptr;
    }
    ui->tositePvmEdit->setFocus();

    // Apurivinkit alkuun
    // Ensimmäisillä kerroilla näytetään erityinen vinkki Apurin käytöstä
    QSettings settings;
    if( settings.value("ApuriVinkki", 1).toInt() > 0)
    {
        if( !apurivinkki_)
            apurivinkki_ = new ApuriVinkki(this);

        apurivinkki_->show();
        apurivinkki_->move( (width() - apurivinkki_->width()) / 2 ,
                            ui->apuriNappi->y() - apurivinkki_->height() - ui->apuriNappi->height() / 2 );
    }
    naytaSummat();
    ui->tabWidget->setCurrentIndex(0);

    if( ui->tositetyyppiCombo->currentData(TositelajiModel::IdRooli).toInt() == 0)
    {
        // Ei oletuksena järjestelmätositetta
        ui->tositetyyppiCombo->setCurrentIndex( ui->tositetyyppiCombo->findData( 1, TositelajiModel::IdRooli) );
    }

}

void KirjausWg::tallenna()
{


    // Ellei yhtään vientiä, näytetään varoitus
    if( model_->vientiModel()->debetSumma() == 0 && model_->vientiModel()->kreditSumma() == 0)
    {
        if( QMessageBox::question(this, tr("Tosite puutteellinen"),
           tr("Tositteeseen ei ole kirjattu yhtään vientiä.\n"
              "Näin voi menetellä esim. liitetietotositteiden kanssa.\n\n"
              "Tallennetaanko tosite ilman vientejä?"),
            QMessageBox::Yes | QMessageBox::Cancel, QMessageBox::Cancel) != QMessageBox::Yes)
            return;
    }

    // Varoitus, jos kirjataan verollisia alv-ilmoituksen antamisen jälkeen
    bool alvvaro = false;

    for(int i=0; i < model()->vientiModel()->rowCount(QModelIndex()); i++)
    {
        QModelIndex indeksi = model()->vientiModel()->index(i,0);
        if(  indeksi.data(VientiModel::AlvKoodiRooli).toInt() > 0 &&
             indeksi.data(VientiModel::PvmRooli).toDate().daysTo( kp()->asetukset()->pvm("AlvIlmoitus")) >= 0  )
            alvvaro = true;

        // Estetään alv-tileille kirjaaminen ilman alv-koodia
        if( indeksi.data(VientiModel::AlvKoodiRooli).toInt() == 0)
        {
            Tili tili = kp()->tilit()->tiliNumerolla( indeksi.data(VientiModel::TiliNumeroRooli).toInt() );
            if( tili.onko(TiliLaji::ALVSAATAVA) || tili.onko(TiliLaji::ALVVELKA))
            {
                QMessageBox::critical(this, tr("Arvonlisäverokoodi puuttuu"),
                                      tr("Tilille %1 %2 on tehty kirjaus, jossa ei ole määritelty arvonlisäveron ohjaustietoja.\n\n"
                                         "Arvonlisäveroon liittyvät kirjaukset on aina määriteltävä oikeilla verokoodeilla, "
                                         "jotta kausiveroilmoitukseen saadaan oikeat tiedot.\n\n"
                                         "Käyttämällä Kirjausapuria saat automaattisesti oikeat arvonlisäveron ohjaustiedot." )
                                      .arg(tili.numero()).arg(tili.nimi()));
                return;
            }
        }


        // #62: Estetään kirjaukset lukitulle tilikaudelle
        if( indeksi.data(VientiModel::PvmRooli).toDate() <= kp()->tilitpaatetty() &&
                indeksi.data(VientiModel::IdRooli).toInt() == 0)
        {
            QMessageBox::critical(this, tr("Ei voi kirjata lukitulle tilikaudelle"),
                                  tr("Kirjaus %1 kohdistuu lukitulle tilikaudelle "
                                     "(kirjanpito lukittu %2 saakka)." )
                                  .arg( indeksi.data(VientiModel::PvmRooli).toDate().toString("dd.MM.yyyy"))
                                  .arg( kp()->tilitpaatetty().toString("dd.MM.yyyy")), QMessageBox::Cancel);
            return;
        }
    }
    if( alvvaro )
    {
        if( QMessageBox::critical(this, tr("Arvonlisäveroilmoitus annettu"),
           tr("Arvonlisäveroilmoitus on annettu %1 saakka.\n\n"
              "Kirjanpitolaki 2. luku 7§ 2. mom:\n"
              "Tositteen, kirjanpidon tai muun kirjanpitoaineiston sisältöä ei saa muuttaa eikä "
              "poistaa sen jälkeen kuin 6§ tarkoitettu (kirjanpidosta viranomaisille verotusta "
              "tai muuta tarkoitusta varten määräajassa tehtävä) ilmoitus on tehty.\n\n"
              "Tallennetaanko tosite silti?").arg( kp()->asetukset()->pvm("AlvIlmoitus").toString("dd.MM.yyyy") ),
            QMessageBox::Yes | QMessageBox::Cancel, QMessageBox::Cancel) != QMessageBox::Yes)
            return;
    }

    // Tallennus

    tiedotModeliin();

    if( !model_->tallenna() )
    {
        // Kirjauksessa virhe
        QSqlError virhe = kp()->tietokanta()->lastError();
        QMessageBox::critical( this, tr("Virhe tallennuksessa"),
                               tr("Tallentaminen ei onnistunut seuraavan tietokantavirheen takia: %1")
                               .arg(virhe.text()));
        return;
    }

    emit kp()->onni(tr("Tosite %1%2/%3 tallennettu")
                    .arg(model_->tositelaji().tunnus())
                    .arg(model_->tunniste())
                    .arg( kp()->tilikausiPaivalle( model_->pvm() ).kausitunnus() ));

    tyhjenna();

    ui->tositePvmEdit->setFocus();

    if( !kp()->asetukset()->onko("EkaTositeKirjattu"))
        kp()->asetukset()->aseta("EkaTositeKirjattu", true);

    emit tositeKasitelty();
}

void KirjausWg::hylkaa()
{
    tyhjenna();
    ui->tositetyyppiCombo->setCurrentIndex(0);
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
                                  tr("Tietokantavirhe tositetta poistettaessa\n\n%1").arg( kp()->tietokanta()->lastError().text() ));
    }
}

void KirjausWg::vientiValittu()
{
    QModelIndex index = ui->viennitView->selectionModel()->currentIndex();
    QDate vientiPvm = index.data(VientiModel::PvmRooli).toDate();
    ui->poistariviNappi->setEnabled( index.isValid() && vientiPvm > kp()->tilitpaatetty());
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
    connect( laskuDlg_, SIGNAL(finished(int)), this, SLOT(tiedotModelista()));

}

void KirjausWg::paivitaTallennaPoistaNapit()
{
    poistaAktio_->setEnabled( model()->muokattu() && model_->id() > -1 && model()->muokkausSallittu());
    uudeksiAktio_->setEnabled( !model()->muokattu() );

    naytaSummat();

}

void KirjausWg::paivitaVaroitukset() const
{
    // Yhdistetty varoitusten näyttäjä
    ui->varoKuva->setPixmap(QPixmap());
    ui->varoTeksti->clear();

    if( kp()->tilitpaatetty() >= kp()->tilikaudet()->kirjanpitoLoppuu() )
    {
        ui->varoKuva->setPixmap(QPixmap(":/pic/stop.png"));
        ui->varoTeksti->setText( tr("Kirjanpidossa ei ole\navointa tilikautta."));
    }
    else if( kp()->tilitpaatetty() >= ui->tositePvmEdit->date() )
    {
        ui->varoKuva->setPixmap( QPixmap(":/pic/lukittu.png"));
        ui->varoTeksti->setText( tr("Kirjanpito lukittu\n%1 saakka").arg(kp()->tilitpaatetty().toString("dd.MM.yyyy")));
    }
    else if( kp()->asetukset()->onko("AlvVelvollinen") && ui->tositePvmEdit->date() <= kp()->asetukset()->pvm("AlvIlmoitus") )
    {
        ui->varoTeksti->setText( tr("Alv-ilmoitus annettu\n%1 saakka").arg(kp()->asetukset()->pvm("AlvIlmoitus").toString("dd.MM.yyyy")));
        ui->varoKuva->setPixmap( QPixmap(":/pic/vero.png"));
    }
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
    dui.alkuSpin->setValue( ui->tunnisteEdit->text().toInt() );

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
        model()->tuloste().tulosta( kp()->printer(), &painter, 0 );
        painter.end();
    }
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
}

int KirjausWg::tiliotetiliId()
{
    if( !ui->tilioteBox->isChecked())
        return 0;
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
                if( watched == ui->tositetyyppiCombo)
                    kirjausApuri();
                else
                    focusNextChild();
                return true;
            }
            // Tositetyypistä pääsee tabulaattorilla uudelle riville
            else if( keyEvent->key() == Qt::Key_Tab && watched == ui->tositetyyppiCombo)
            {
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
            keyEvent->key() == Qt::Key_Tab) &&
                keyEvent->modifiers() == Qt::NoModifier )
        {

            if( ui->viennitView->currentIndex().column() == VientiModel::SELITE &&
                ui->viennitView->currentIndex().row() == model()->vientiModel()->rowCount(QModelIndex()) - 1 )
            {
                lisaaRivi();
                ui->viennitView->setCurrentIndex( model()->vientiModel()->index( model()->vientiModel()->rowCount(QModelIndex())-1, VientiModel::TILI ) );
                return true;
            }

            else if( ui->viennitView->currentIndex().column() == VientiModel::TILI )
            {
                ui->viennitView->setCurrentIndex( ui->viennitView->currentIndex().siblingAtColumn(VientiModel::DEBET) );
                Tili tili = kp()->tilit()->tiliIdlla( ui->viennitView->currentIndex().data(VientiModel::TiliIdRooli).toInt() );
                if( tili.onko(TiliLaji::MENO))
                    ui->viennitView->setCurrentIndex( ui->viennitView->currentIndex().siblingAtColumn(VientiModel::KREDIT) );
                return true;
            }

            else if( ui->viennitView->currentIndex().column() == VientiModel::DEBET)
            {
                ui->viennitView->setCurrentIndex( ui->viennitView->currentIndex().siblingAtColumn(VientiModel::KREDIT) );
                if( ui->viennitView->currentIndex().data(VientiModel::DebetRooli).toInt()  )
                {
                    ui->viennitView->setCurrentIndex( ui->viennitView->currentIndex().siblingAtColumn(VientiModel::KOHDENNUS) );
                    // Enterillä suoraan uusi rivi
                    if( ( keyEvent->key() == Qt::Key_Enter || keyEvent->key() == Qt::Key_Return )
                            && ui->viennitView->currentIndex().row() == model()->vientiModel()->rowCount(QModelIndex()) - 1 )
                    {
                        lisaaRivi();
                        ui->viennitView->setCurrentIndex( model()->vientiModel()->index( model()->vientiModel()->rowCount(QModelIndex())-1, VientiModel::TILI ) );
                    }
                }
                return true;
            }
            else if( ui->viennitView->currentIndex().column() < VientiModel::ALV)
            {
                // Enterillä pääsee suoraan seuraavalle riville
                ui->viennitView->setCurrentIndex( ui->viennitView->currentIndex().siblingAtColumn( ui->viennitView->currentIndex().column()+1)  );
                if( ( ui->viennitView->currentIndex().data(VientiModel::KreditRooli).toInt()  || ui->viennitView->currentIndex().data(VientiModel::DebetRooli).toInt() ) &&
                    ( keyEvent->key() == Qt::Key_Enter || keyEvent->key() == Qt::Key_Return ) &&
                      ui->viennitView->currentIndex().row() == model()->vientiModel()->rowCount(QModelIndex()) - 1 )
                {
                    lisaaRivi();
                    ui->viennitView->setCurrentIndex( model()->vientiModel()->index( model()->vientiModel()->rowCount(QModelIndex())-1, VientiModel::TILI ) );
                }
                return true;
            }


        }
    }


    return QWidget::eventFilter(watched, event);
}

void KirjausWg::naytaSummat()
{
    qlonglong debet = model_->vientiModel()->debetSumma();
    qlonglong kredit = model_->vientiModel()->kreditSumma();

    qlonglong erotus = qAbs( debet - kredit );

    if( erotus )
        ui->summaLabel->setText( tr("Debet %L1 €    Kredit %L2 €    <b>Erotus %L3 €</b>")
                                 .arg((1.0 * debet )/100.0 ,0,'f',2)
                                 .arg((1.0 * kredit ) / 100.0 ,0,'f',2)
                                 .arg((1.0 * erotus ) / 100.0 ,0,'f',2) );
    else
        ui->summaLabel->setText( tr("Debet %L1 €    Kredit %L2 €")
                                 .arg((1.0 * debet )/100.0 ,0,'f',2)
                                 .arg((1.0 * kredit ) / 100.0 ,0,'f',2));

    // #39: Debet- ja kredit-kirjausten on täsmättävä
    ui->tallennaButton->setEnabled( !erotus && model()->muokattu() && model()->muokkausSallittu() &&
                                    model()->kelpaakoTunniste( ui->tunnisteEdit->text().toInt() ));

    // Tilien joilla kirjauksia oltava valideja
    for(int i=0; i < model_->vientiModel()->rowCount(QModelIndex()); i++)
    {
        QModelIndex index = model_->vientiModel()->index(i,0);
        if( index.data(VientiModel::TiliNumeroRooli).toInt() == 0 &&
                ( index.data(VientiModel::DebetRooli).toLongLong() != 0 ||
                  index.data(VientiModel::KreditRooli).toLongLong() != 0))
        {
            // Nollatilille saa kuitenkin kirjata maksuperusteisen laskun maksamisen
            if( !index.data(VientiModel::LaskuPvmRooli).toDate().isValid() &&
                !index.data(VientiModel::EraIdRooli).toInt())
                    ui->tallennaButton->setEnabled(false);
        }
    }

}

void KirjausWg::lataaTosite(int id)
{
    model_->lataa(id);

    tiedotModelista();

    ui->tabWidget->setCurrentIndex(0);
    ui->tositePvmEdit->setFocus();

    if( model_->liiteModel()->rowCount(QModelIndex()))
        ui->liiteView->setCurrentIndex( model_->liiteModel()->index(0) );

    // Jos tositteella yksikin lukittu vienti, ei voi poistaa
    poistaAktio_->setEnabled(model()->muokkausSallittu() &&
                                model()->id() > -1);

    for(int i = 0; i < model_->vientiModel()->rowCount(QModelIndex()); i++)
    {
        QModelIndex index = model_->vientiModel()->index(i,0);
        if( index.data(VientiModel::PvmRooli).toDate() <= kp()->tilitpaatetty())
            poistaAktio_->setEnabled(false);
    }

    ui->poistaLiiteNappi->setEnabled( model()->liiteModel()->rowCount(QModelIndex()) );

}

void KirjausWg::paivitaKommenttiMerkki()
{
    if( ui->kommentitEdit->document()->toPlainText().isEmpty())
    {
        ui->tabWidget->setTabIcon(1, QIcon());
    }
    else
    {
        ui->tabWidget->setTabIcon(1, QIcon(":/pic/kommentti.png"));
    }
    model_->asetaKommentti( ui->kommentitEdit->toPlainText() );

}

void KirjausWg::paivitaTunnisteVari()
{
    bool kelpaako = model_->kelpaakoTunniste( ui->tunnisteEdit->text().toInt()) ;

    if( kelpaako)
    {
        ui->tunnisteEdit->setStyleSheet("color: black;");
        model()->asetaTunniste(ui->tunnisteEdit->text().toInt());
    }
    else
        ui->tunnisteEdit->setStyleSheet("color: red;");

    ui->siiraNumerotBtn->setVisible( !kelpaako );

    paivitaTallennaPoistaNapit();
}

void KirjausWg::lisaaLiite(const QString polku)
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
        ui->poistaLiiteNappi->setEnabled(true);

    }

}

void KirjausWg::lisaaLiite()
{
    lisaaLiite(QFileDialog::getOpenFileName(this, tr("Lisää liite"),QString(),tr("Pdf-tiedosto (*.pdf);;Kuvat (*.png *.jpg);;CSV-tiedosto (*.csv);;Kaikki tiedostot (*.*)")));
}


void KirjausWg::tiedotModeliin()
{
    model_->asetaPvm( ui->tositePvmEdit->date());
    model_->asetaOtsikko( ui->otsikkoEdit->text());
    model_->asetaTunniste( ui->tunnisteEdit->text().toInt());
    model_->asetaTositelaji( ui->tositetyyppiCombo->currentData( TositelajiModel::IdRooli).toInt() );

    model_->asetaKommentti( ui->kommentitEdit->toPlainText() );

    if( ui->tilioteBox->isChecked())
    {
        model_->asetaTiliotetili( ui->tiliotetiliCombo->currentData(TiliModel::IdRooli).toInt() );
        model_->json()->set("TilioteAlkaa", ui->tiliotealkaenEdit->date());
        model_->json()->set("TilioteLoppuu", ui->tilioteloppuenEdit->date());
    }
    else
    {
        model_->asetaTiliotetili(0);
        model_->json()->unset("TilioteAlkaa");
        model_->json()->unset("TilioteLoppuu");
    }
    paivitaTallennaPoistaNapit();
}

void KirjausWg::tiedotModelista()
{
    salliMuokkaus( model_->muokkausSallittu() );

    ui->tositePvmEdit->setDate( model_->pvm() );
    ui->otsikkoEdit->setText( model_->otsikko() );
    ui->kommentitEdit->setPlainText( model_->kommentti());
    ui->tunnisteEdit->setText( QString::number(model_->tunniste()));
    ui->tositetyyppiCombo->setCurrentIndex( ui->tositetyyppiCombo->findData( model_->tositelaji().id(), TositelajiModel::IdRooli ) );
    ui->kausiLabel->setText(QString("/ %1").arg( kp()->tilikaudet()->tilikausiPaivalle(model_->pvm()).kausitunnus() ));

    ui->tilioteBox->setChecked( model_->tiliotetili() != 0 );
    // Tiliotetilin yhdistämiset!
    if( model_->tiliotetili())
    {
        ui->tiliotetiliCombo->setCurrentIndex( ui->tiliotetiliCombo->findData( QVariant(model_->tiliotetili()) ,TiliModel::IdRooli ) );
        ui->tiliotealkaenEdit->setDate( model_->json()->date("TilioteAlkaa") );
        ui->tilioteloppuenEdit->setDate( model_->json()->date("TilioteLoppuu"));
    }
    paivitaVaroitukset();

    if( model()->id() > 0)
    {
        ui->tunnisteLabel->setText( QString("%1").arg( model_->id(), 8, 10, QChar('0') ) );
        ui->luotuLabel->setText( model_->luontiAika().toString("dd.MM.yyyy hh.mm.ss") );
        ui->muokattuLabel->setText( model_->muokattuAika().toString("dd.MM.yyyy hh.mm.ss"));

    }
    else
    {
        ui->tunnisteLabel->setText( tr("Uusi tosite"));
        ui->luotuLabel->clear();
        ui->muokattuLabel->clear();
    }


}

void KirjausWg::salliMuokkaus(bool sallitaanko)
{
    ui->tositePvmEdit->setEnabled(sallitaanko);
    ui->tositetyyppiCombo->setEnabled(sallitaanko);
    ui->kommentitEdit->setEnabled(sallitaanko);
    ui->tunnisteEdit->setEnabled(sallitaanko);
    ui->otsikkoEdit->setEnabled(sallitaanko);
    ui->lisaaliiteNappi->setEnabled(sallitaanko);
    ui->poistaLiiteNappi->setEnabled(sallitaanko);

    if(sallitaanko)
        ui->tositePvmEdit->setDateRange( kp()->tilitpaatetty().addDays(1), kp()->tilikaudet()->kirjanpitoLoppuu() );
    else
        ui->tositePvmEdit->setDateRange( kp()->tilikaudet()->kirjanpitoAlkaa(), kp()->tilikaudet()->kirjanpitoLoppuu() );


}

void KirjausWg::vaihdaTositeTyyppi()
{
    model_->asetaTositelaji( ui->tositetyyppiCombo->currentData(TositelajiModel::IdRooli).toInt() );
    ui->tyyppiLabel->setText( model_->tositelaji().tunnus());

    // Päivitetään tositenumero modelista ;)
    ui->tunnisteEdit->setText( QString::number(model_->tunniste() ));

    // Jos tositelaji kirjaa tiliotteita, aktivoidaan tiliotteen kirjaaminen
    if( model_->tositelaji().json()->luku("Kirjaustyyppi") == TositelajiModel::TILIOTE)
    {
        Tili otetili = kp()->tilit()->tiliNumerolla( model_->tositelaji().json()->luku("Vastatili") );
        ui->tiliotetiliCombo->setCurrentIndex( ui->tiliotetiliCombo->findData(otetili.id(), TiliModel::IdRooli)  );
        model_->asetaTiliotetili( otetili.id() );
        ui->tilioteBox->setChecked(true);
        if( ui->otsikkoEdit->text().isEmpty())
            ui->otsikkoEdit->setText( QString("Tiliote %1 ajalta %2 - %3")
                    .arg(otetili.nimi()).arg(ui->tiliotealkaenEdit->date().toString("dd.MM.yyyy"))
                    .arg( ui->tilioteloppuenEdit->date().toString("dd.MM.yyyy")));
        tiedotModeliin();
    }
    else
    {
        model_->asetaTiliotetili(0);
        ui->tilioteBox->setChecked(false);
    }

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

void KirjausWg::kirjausApuri()
{
    // Apurivinkki näytetään, kunnes Apuria on käytetty kolme kertaa
    if( apurivinkki_ )
    {
        QSettings settings;
        if( settings.value("ApuriVinkki", 3).toInt())
            settings.setValue("ApuriVinkki", settings.value("ApuriVinkki",3).toInt() - 1 );
        apurivinkki_->hide();
    }

    KirjausApuriDialog *dlg = new KirjausApuriDialog( model_, this);
    dlg->show();

}

void KirjausWg::pvmVaihtuu()
{
    if( !model_->muokkausSallittu() )
        return;

    QDate paiva = ui->tositePvmEdit->date();
    QDate vanhaPaiva = model_->pvm();

    model_->asetaPvm(paiva);

    // Tiliotepäiväyksen kirjauksen kuukauden alkuun ja loppuun
    QDate alkupaiva = paiva.addDays( 1 - paiva.day() );   // Siirretään kuukauden alkuun
    ui->tiliotealkaenEdit->setDate( alkupaiva );
    QDate loppupaiva = alkupaiva.addMonths(1).addDays(-1); // Siirrytään kuukauden loppuun
    ui->tilioteloppuenEdit->setDate(loppupaiva);

    if( kp()->tilikaudet()->tilikausiPaivalle(paiva).alkaa() != kp()->tilikaudet()->tilikausiPaivalle(vanhaPaiva).alkaa())
    {
        // Siirrytty toiselle tilikaudelle, vaihdetaan numerointia
        model_->asetaTunniste( model_->seuraavaTunnistenumero());
        ui->tunnisteEdit->setText( QString::number(model_->tunniste() ));
        ui->kausiLabel->setText( QString("/ %1").arg(kp()->tilikaudet()->tilikausiPaivalle(paiva).kausitunnus() ));
    }
    paivitaVaroitukset();
}

void KirjausWg::naytaLiite()
{
    QModelIndex index = ui->liiteView->currentIndex();
    if( index.isValid())
        PdfIkkuna::naytaPdf( index.data(LiiteModel::PdfRooli).toByteArray() );

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

