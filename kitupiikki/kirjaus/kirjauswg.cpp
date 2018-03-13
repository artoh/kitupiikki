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

#include "kirjauswg.h"
#include "tilidelegaatti.h"
#include "eurodelegaatti.h"
#include "pvmdelegaatti.h"
#include "kirjausapuridialog.h"
#include "kohdennusdelegaatti.h"
#include "taseeravalintadialogi.h"

#include "verodialogi.h"

#include "db/kirjanpito.h"
#include "laskutus/laskunmaksudialogi.h"

#include "tuonti/tuonti.h"

#include <QDebug>
#include <QSqlQuery>
#include <QSqlError>
#include <QMessageBox>
#include <QIntValidator>
#include <QFileDialog>
#include <QDesktopServices>
#include <QUrl>

#include <QShortcut>

#include <QSortFilterProxyModel>

KirjausWg::KirjausWg(TositeModel *tositeModel, QWidget *parent)
    : QWidget(parent), model_(tositeModel), laskuDlg_(0)
{
    ui = new Ui::KirjausWg();
    ui->setupUi(this);

    ui->viennitView->setModel( model_->vientiModel() );

    connect( model_->vientiModel() , SIGNAL(siirryRuutuun(QModelIndex)), ui->viennitView, SLOT(setCurrentIndex(QModelIndex)));
    connect( model_->vientiModel(), SIGNAL(siirryRuutuun(QModelIndex)), ui->viennitView, SLOT(edit(QModelIndex)));
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
    connect( ui->poistaNappi, SIGNAL(clicked(bool)), this, SLOT(poistaTosite()));

    ui->tositetyyppiCombo->setModel( Kirjanpito::db()->tositelajit());
    ui->tositetyyppiCombo->setModelColumn( TositelajiModel::NIMI);

    // Kun tositteen päivää vaihdetaan, vaihtuu myös tiliotepäivät.
    // Siksi tosipäivä ladattava aina ennen tiliotepäiviä!
    connect( ui->tositePvmEdit, SIGNAL(editingFinished()), this, SLOT(pvmVaihtuu()));

    connect( ui->tositetyyppiCombo, SIGNAL(currentIndexChanged(int)), this, SLOT(vaihdaTositeTyyppi()));
    connect( ui->tunnisteEdit, SIGNAL(textEdited(QString)), this, SLOT(paivitaTunnisteVari()));
    connect( ui->otsikkoEdit, SIGNAL(textEdited(QString)), model_, SLOT(asetaOtsikko(QString)));
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


    // Enterillä päiväyksestä eteenpäin
    ui->tositePvmEdit->installEventFilter(this);
    ui->otsikkoEdit->installEventFilter(this);
    ui->tositetyyppiCombo->installEventFilter(this);
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
    model_->vientiModel()->lisaaVienti();
    ui->viennitView->setFocus();

    QModelIndex indeksi = model_->vientiModel()->index( model_->vientiModel()->rowCount(QModelIndex()) - 1, VientiModel::TILI );

    ui->viennitView->setCurrentIndex( indeksi  );

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
    ui->poistaNappi->setEnabled(false);
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
        laskuDlg_ = 0;
    }
    ui->tositePvmEdit->setFocus();
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

        // #62: Estetään kirjaukset lukitulle tilikaudelle
        if( indeksi.data(VientiModel::PvmRooli).toDate() <= kp()->tilitpaatetty() &&
                indeksi.data(VientiModel::IdRooli).toInt() == 0)
        {
            QMessageBox::critical(this, tr("Ei voi kirjata lukitulle tilikaudelle"),
                                  tr("Kirjaus %1 kohdistuu lukitulle tilikaudelle "
                                     "(kirjanpito lukittu %2 saakka)." )
                                  .arg( indeksi.data(VientiModel::PvmRooli).toDate().toString(Qt::SystemLocaleShortDate))
                                  .arg( kp()->tilitpaatetty().toString(Qt::SystemLocaleShortDate)), QMessageBox::Cancel);
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
              "Tallennetaanko tosite silti?").arg( kp()->asetukset()->pvm("AlvIlmoitus").toString(Qt::SystemLocaleShortDate) ),
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
    tyhjenna();
    emit tositeKasitelty();

    ui->tositePvmEdit->setFocus();
    emit kp()->onni("Tosite tallennettu");

    if( !kp()->asetukset()->onko("EkaTositeKirjattu"))
        kp()->asetukset()->aseta("EkaTositeKirjattu", true);
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
        model()->poista();
        tyhjenna();
        emit tositeKasitelty();
    }
}

void KirjausWg::vientiValittu()
{
    QModelIndex index = ui->viennitView->selectionModel()->currentIndex();
    QDate vientiPvm = index.data(VientiModel::PvmRooli).toDate();
    ui->poistariviNappi->setEnabled( index.isValid() && vientiPvm > kp()->tilitpaatetty());
}

void KirjausWg::vientivwAktivoitu(QModelIndex indeksi)
{
    // Tehdään alv-kirjaus
    if( model()->muokkausSallittu() )
    {

        if(indeksi.column() == VientiModel::ALV )
        {
            VeroDialogi verodlg(this);
            if( verodlg.exec( indeksi.data(VientiModel::AlvKoodiRooli).toInt(), indeksi.data(VientiModel::AlvProsenttiRooli).toInt() ))
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

    laskuDlg_->exec();

}

void KirjausWg::paivitaTallennaPoistaNapit()
{
    ui->tallennaButton->setEnabled( model()->muokattu() && model()->muokkausSallittu() );
    ui->poistaNappi->setEnabled( model()->muokattu() && model_->id() > -1 && model()->muokkausSallittu());
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
                                 .arg(((double) debet )/100.0 ,0,'f',2)
                                 .arg(((double) kredit ) / 100.0 ,0,'f',2)
                                 .arg(((double) erotus ) / 100.0 ,0,'f',2) );
    else
        ui->summaLabel->setText( tr("Debet %L1 €    Kredit %L2 €")
                                 .arg(((double) debet )/100.0 ,0,'f',2)
                                 .arg(((double) kredit ) / 100.0 ,0,'f',2));

    // #39: Debet- ja kredit-kirjausten on täsmättävä
    ui->tallennaButton->setEnabled( !erotus && model()->muokattu() && model()->muokkausSallittu() );

    // Tilien joilla kirjauksia oltava valideja
    for(int i=0; i < model_->vientiModel()->rowCount(QModelIndex()); i++)
    {
        QModelIndex index = model_->vientiModel()->index(i,0);
        if( index.data(VientiModel::TiliNumeroRooli).toInt() == 0 &&
                ( index.data(VientiModel::DebetRooli).toLongLong() != 0 ||
                  index.data(VientiModel::KreditRooli).toLongLong() != 0))
            ui->tallennaButton->setEnabled(false);
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
    ui->poistaNappi->setEnabled(model()->muokkausSallittu() &&
                                model()->id() > -1);

    for(int i = 0; i < model_->vientiModel()->rowCount(QModelIndex()); i++)
    {
        QModelIndex index = model_->vientiModel()->index(i,0);
        if( index.data(VientiModel::PvmRooli).toDate() <= kp()->tilitpaatetty())
            ui->poistaNappi->setEnabled(false);
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

}

void KirjausWg::paivitaTunnisteVari()
{
    if( model_->kelpaakoTunniste( ui->tunnisteEdit->text().toInt() ))
        ui->tunnisteEdit->setStyleSheet("color: black;");
    else
        ui->tunnisteEdit->setStyleSheet("color: red;");

}

void KirjausWg::lisaaLiite(const QString polku)
{
    if( !polku.isEmpty())
    {
        // Pyritään ensin tuomaan
        if( !Tuonti::tuo(polku, this))
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
    lisaaLiite(QFileDialog::getOpenFileName(this, tr("Lisää liite"),QString(),tr("Pdf-tiedosto (*.pdf);;Kuvat (*.png *.jpg);;CSV-tiedosto (*.csv)")));
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

    // Yhdistetty varoitusten näyttäjä
    ui->varoKuva->setPixmap(QPixmap());
    ui->varoTeksti->clear();

    if( kp()->tilitpaatetty() >= kp()->tilikaudet()->kirjanpitoLoppuu() )
    {
        ui->varoKuva->setPixmap(QPixmap(":/pic/stop.png"));
        ui->varoTeksti->setText( tr("Kirjanpidossa ei ole\navointa tilikautta."));
    }
    else if( kp()->tilitpaatetty() >= model_->pvm() )
    {
        ui->varoKuva->setPixmap( QPixmap(":/pic/lukittu.png"));
        ui->varoTeksti->setText( tr("Kirjanpito lukittu\n%1 saakka").arg(kp()->tilitpaatetty().toString(Qt::SystemLocaleShortDate)));
    }
    else if( kp()->asetukset()->onko("AlvVelvollinen") && model_->pvm() <= kp()->asetukset()->pvm("AlvIlmoitus") )
    {
        ui->varoTeksti->setText( tr("Alv-ilmoitus annettu\n%1 saakka").arg(kp()->asetukset()->pvm("AlvIlmoitus").toString(Qt::SystemLocaleShortDate)));
        ui->varoKuva->setPixmap( QPixmap(":/pic/vero.png"));
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
                    .arg(otetili.nimi()).arg(ui->tiliotealkaenEdit->date().toString(Qt::SystemLocaleShortDate))
                    .arg( ui->tilioteloppuenEdit->date().toString(Qt::SystemLocaleShortDate)));
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
    KirjausApuriDialog dlg( model_, this);
    dlg.exec();
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
}

void KirjausWg::naytaLiite()
{
    if( ui->liiteView->currentIndex().isValid())
        QDesktopServices::openUrl( QUrl::fromLocalFile( ui->liiteView->currentIndex().data(LiiteModel::PolkuRooli).toString() ) );

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

