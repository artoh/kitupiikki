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
#include "laskuntulostaja.h"
#include "laskutusverodelegaatti.h"
#include "laskuryhmamodel.h"
#include "ryhmantuontidlg.h"
#include "ryhmantuontimodel.h"

#include "kirjaus/eurodelegaatti.h"
#include "kirjaus/kohdennusdelegaatti.h"
#include "kirjaus/tilidelegaatti.h"

#include "kirjaus/verodialogi.h"
#include "naytin/naytinikkuna.h"
#include "validator/ytunnusvalidator.h"
#include "asiakkaatmodel.h"
#include "ryhmaasiakasproxy.h"

#include "finvoice.h"

#include "ui_yhteystiedot.h"

#include "validator/ytunnusvalidator.h"

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

#include <QSettings>
#include <QRegularExpression>

#include <QMessageBox>
#include <QPdfWriter>


LaskuDialogi::LaskuDialogi(LaskuModel *laskumodel) :
    model(laskumodel), ui( new Ui::LaskuDialogi)
{
    laskuIkkunoita__++;
    tulostaja = new LaskunTulostaja(model);

    if( !laskumodel)
        model = new LaskuModel();

    ui->setupUi(this);

    setAttribute(Qt::WA_DeleteOnClose);
    resize(800,600);
    restoreGeometry( kp()->settings()->value("LaskuDialogi").toByteArray());

    ui->perusteCombo->addItem(QIcon(":/pic/suorite.png"), tr("Suoriteperusteinen"), LaskuModel::SUORITEPERUSTE);
    ui->perusteCombo->addItem(QIcon(":/pic/kirje.png"), tr("Laskutusperusteinen"), LaskuModel::LASKUTUSPERUSTE);
    ui->perusteCombo->addItem(QIcon(":/pic/euro.png"), tr("Maksuperusteinen"), LaskuModel::MAKSUPERUSTE);
    ui->perusteCombo->addItem(QIcon(":/pic/kateinen.png"), tr("Käteiskuitti"), LaskuModel::KATEISLASKU);

    ui->kieliCombo->addItem(QIcon(":/pic/fi.png"), tr("Suomi"), "FI");
    ui->kieliCombo->addItem(QIcon(":/pic/se.png"), tr("Ruotsi"), "SE");
    ui->kieliCombo->addItem(QIcon(":/pic/en.png"), tr("Englanti"), "EN");

    model->lisaaRivi();

    tuotteet = kp()->tuotteet();
    tuoteProxy = new QSortFilterProxyModel(this);
    tuoteProxy->setSourceModel(tuotteet);
    ui->tuotelistaView->setModel(tuoteProxy);
    ui->tuotelistaView->sortByColumn(TuoteModel::NIMIKE, Qt::AscendingOrder);
    tuoteProxy->setSortLocaleAware(true);
    connect( ui->tuotehakuEdit, SIGNAL(textChanged(QString)), tuoteProxy, SLOT(setFilterFixedString(QString)) );

    ui->nroLabel->setText( QString::number(model->laskunro()));
    ui->ytunnus->setValidator( new YTunnusValidator(true));

    ui->rivitView->setModel(model);
    ui->rivitView->setItemDelegateForColumn(LaskuModel::AHINTA, new EuroDelegaatti());
    ui->rivitView->setItemDelegateForColumn(LaskuModel::TILI, new TiliDelegaatti());

    kohdennusDelegaatti = new KohdennusDelegaatti();
    ui->rivitView->setItemDelegateForColumn(LaskuModel::KOHDENNUS, kohdennusDelegaatti );
    connect( ui->toimitusDate, SIGNAL(dateChanged(QDate)), kohdennusDelegaatti, SLOT(asetaKohdennusPaiva(QDate)));


    ui->rivitView->setItemDelegateForColumn(LaskuModel::BRUTTOSUMMA, new EuroDelegaatti());
    ui->rivitView->setItemDelegateForColumn(LaskuModel::ALV, new LaskutusVeroDelegaatti());

    ui->rivitView->setColumnHidden( LaskuModel::ALV, !kp()->asetukset()->onko("AlvVelvollinen") );
    ui->rivitView->setColumnHidden( LaskuModel::KOHDENNUS, kp()->kohdennukset()->rowCount(QModelIndex()) < 2);

    ui->naytaNappi->setChecked( kp()->asetukset()->onko("LaskuNaytaTuotteet") );
    ui->tabWidget->setTabEnabled(VERKKOLASKU, false);


    // Laitetaan täydentäjä nimen syöttöön
    QCompleter *nimiTaydentaja = new QCompleter(this);
    QSqlQueryModel *sqlmalli = new QSqlQueryModel(this);
    sqlmalli->setQuery("select distinct asiakas from vienti order by asiakas");
    nimiTaydentaja->setModel(sqlmalli);
    nimiTaydentaja->setModelSorting(QCompleter::CaseSensitivelySortedModel);
    ui->saajaEdit->setCompleter(nimiTaydentaja);


    ui->rivitView->horizontalHeader()->setSectionResizeMode(LaskuModel::NIMIKE, QHeaderView::Stretch);
    ui->tuotelistaView->horizontalHeader()->setSectionResizeMode(TuoteModel::NIMIKE, QHeaderView::Stretch);
    ui->ryhmaView->setSelectionBehavior(QTableView::SelectRows);

    ui->perusteCombo->setCurrentIndex( model->kirjausperuste() );

    ui->perusteCombo->setEnabled( model->tyyppi() == LaskuModel::LASKU || model->tyyppi() == LaskuModel::RYHMALASKU);
    ui->saajaEdit->setText( model->laskunsaajanNimi() );
    if( !model->osoite().isEmpty())
        ui->osoiteEdit->setPlainText( model->osoite());
    if( !model->email().isEmpty())
        ui->emailEdit->setText( model->email());
    if( !model->ytunnus().isEmpty())
        ui->ytunnus->setText( model->ytunnus());

    ui->eraDate->setDate( model->erapaiva() );
    ui->eraDate->setEnabled( model->tyyppi() != LaskuModel::HYVITYSLASKU );
    ui->toimitusDate->setDate( model->toimituspaiva());
    ui->perusteCombo->setCurrentIndex( ui->perusteCombo->findData( model->kirjausperuste() ));
    ui->lisatietoEdit->setPlainText( model->lisatieto() );
    ui->asViiteEdit->setText( model->asiakkaanViite());
    ui->verkkoOsoiteEdit->setText( model->verkkolaskuOsoite());
    ui->verkkoValittajaEdit->setText( model->verkkolaskuValittaja());


    if( model->tyyppi() == LaskuModel::HYVITYSLASKU)
    {
        setWindowTitle( tr("Hyvityslasku"));
        ui->toimituspvmLabel->setText( tr("Hyvityspvm"));
        ui->rahaTiliEdit->valitseTiliIdlla( model->viittausLasku().tiliid );
        ui->rahaTiliEdit->setEnabled(false);
        ui->verkkolaskuNappi->setVisible(false);
        connect( ui->ohjeNappi, &QPushButton::clicked, [] { kp()->ohje("laskutus/uusi#hyvityslasku");});
    }
    else if( model->tyyppi() == LaskuModel::MAKSUMUISTUTUS)
    {
        setWindowTitle( tr("Maksumuistutus"));
        ui->rahaTiliEdit->valitseTiliIdlla( model->viittausLasku().tiliid );
        ui->rahaTiliEdit->setEnabled(false);
        ui->verkkolaskuNappi->setVisible(false);
        ui->tallennaNappi->setEnabled(true);
        connect( ui->ohjeNappi, &QPushButton::clicked, [] { kp()->ohje("laskutus/muistutus");});

    }
    else if( model->tyyppi() == LaskuModel::LASKU || model->tyyppi() == LaskuModel::RYHMALASKU)
    {
        ui->eraDate->setMinimumDate( kp()->paivamaara() );
        perusteVaihtuu();
        ui->verkkolaskuNappi->setVisible( kp()->asetukset()->onko("VerkkolaskuKaytossa") );

        if( model->tyyppi() == LaskuModel::LASKU)
            connect( ui->ohjeNappi, &QPushButton::clicked, [] { kp()->ohje("laskutus/uusi");});
        else
            connect( ui->ohjeNappi, &QPushButton::clicked, [] { kp()->ohje("laskutus/ryhma");});
    }

    if( model->tyyppi() == LaskuModel::RYHMALASKU)
    {
        ui->tabWidget->removeTab(VERKKOLASKU);

        setWindowTitle("Ryhmän laskutus");

        ryhmaProxy_ = new QSortFilterProxyModel(this);
        ryhmaProxy_->setSourceModel( model->ryhmaModel());
        ui->ryhmaView->setModel( ryhmaProxy_ );
        ui->ryhmaView->setSortingEnabled(true);

        AsiakkaatModel *asiakkaat = new AsiakkaatModel(model);
        asiakkaat->paivita();

        // Asiakasluettelo suodatetaan ryhmäproxyn läpi
        RyhmaAsiakasProxy *ryhmaAsiakasProxy = new RyhmaAsiakasProxy(this);
        ryhmaAsiakasProxy->asetaRyhmaModel( model->ryhmaModel() );
        ryhmaAsiakasProxy->setSourceModel(asiakkaat);
        ryhmaAsiakasProxy->setFilterCaseSensitivity(Qt::CaseInsensitive);

        ui->asiakasLista->setModel( ryhmaAsiakasProxy );
        connect( ui->assuodatusEdit, &QLineEdit::textChanged, [ryhmaAsiakasProxy] (const QString& teksti) { ryhmaAsiakasProxy->setFilterFixedString(teksti); });

        ui->saajaEdit->hide();
        ui->osoiteEdit->hide();
        ui->emailEdit->hide();
        ui->ytunnus->hide();
        ui->viiteLabel->hide();
        ui->nroLabel->hide();
        ui->asViiteEdit->hide();

        ui->esikatseluNappi->setEnabled(false);
        ui->tulostaNappi->setEnabled(false);
        ui->spostiNappi->setEnabled(false);
        ui->tallennaNappi->setEnabled(false);

        connect( ui->ryhmaView->selectionModel(), &QItemSelectionModel::selectionChanged, this, &LaskuDialogi::ryhmaNapit);
        connect( ui->poistaRyhmastaNappi, &QPushButton::clicked, this, &LaskuDialogi::poistaValitutAsiakkaat);
        connect( ui->tuoRyhmaanNappi, &QPushButton::clicked, this, &LaskuDialogi::tuoAsiakkaitaTiedostosta);

        connect( ui->ohjeNappi, &QPushButton::clicked, [] { kp()->ohje("laskutus/ryhma");});
        ui->ryhmaView->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);

        connect( model->ryhmaModel(), &LaskuRyhmaModel::rowsInserted, this, &LaskuDialogi::paivitaRyhmanTallennusNappi);
        connect( model->ryhmaModel(), &LaskuRyhmaModel::rowsRemoved, this, &LaskuDialogi::paivitaRyhmanTallennusNappi);

    }
    else
    {
        ui->tabWidget->removeTab(RYHMAT);
        if( !kp()->asetukset()->onko("VerkkolaskuKaytossa"))
            ui->tabWidget->removeTab(VERKKOLASKU);

    }

    connect( ui->lisaaNappi, SIGNAL(clicked(bool)), model, SLOT(lisaaRivi()));
    connect( ui->poistaNappi, SIGNAL(clicked(bool)), this, SLOT(poistaLaskuRivi()));
    connect( ui->tulostaNappi, SIGNAL(clicked(bool)), this, SLOT(tulostaLasku()));
    connect( ui->esikatseluNappi, SIGNAL(clicked(bool)), this, SLOT(esikatsele()));
    connect( ui->spostiNappi, SIGNAL(clicked(bool)), this, SLOT(lahetaSahkopostilla()));

    connect( model, &LaskuModel::summaMuuttunut, this, &LaskuDialogi::paivitaSumma);
    connect( ui->perusteCombo, SIGNAL(currentIndexChanged(int)), this, SLOT(perusteVaihtuu()));
    connect( ui->kieliCombo, &QComboBox::currentTextChanged, [this]() { this->model->asetaKieli( this->ui->kieliCombo->currentData().toString()); } );

    ui->kieliCombo->setCurrentIndex( ui->kieliCombo->findData( model->kieli() ) );

    connect( ui->saajaEdit, SIGNAL(textChanged(QString)), this, SLOT(haeOsoite()));
    connect( ui->rivitView, SIGNAL(customContextMenuRequested(QPoint)), this, SLOT(rivienKontekstiValikko(QPoint)));



    connect( ui->tuotelistaView, SIGNAL(clicked(QModelIndex)), this, SLOT(lisaaTuote(QModelIndex)));
    connect( ui->tuotelistaView, SIGNAL(customContextMenuRequested(QPoint)), this, SLOT(tuotteidenKonteksiValikko(QPoint)));
    connect( ui->emailEdit, SIGNAL(textChanged(QString)), this, SLOT(onkoPostiKaytossa()));

    connect( ui->asiakasLista, &QListView::clicked, this, &LaskuDialogi::lisaaAsiakasListalta);
    connect( ui->lisaaRyhmaanNappi, &QPushButton::clicked, this, &LaskuDialogi::lisaaAsiakas);

    connect( ui->ytunnus, &QLineEdit::textChanged, this, &LaskuDialogi::ytunnusSyotetty);
    connect( ui->verkkoOsoiteEdit, &QLineEdit::textChanged, this, &LaskuDialogi::verkkolaskuKayttoon);
    connect( ui->verkkoValittajaEdit, &QLineEdit::textChanged, this, &LaskuDialogi::verkkolaskuKayttoon);
    connect( ui->verkkolaskuNappi, &QPushButton::clicked, this, &LaskuDialogi::finvoice);

    connect( ui->toimitusDate, SIGNAL(dateChanged(QDate)), model, SLOT(asetaToimituspaiva(QDate)));
    connect(ui->eraDate, SIGNAL(dateChanged(QDate)), model, SLOT(asetaErapaiva(QDate)));
    connect( model, &LaskuModel::laskuaMuokattu, ui->tallennaNappi, &QPushButton::setEnabled );


    paivitaTuoteluettelonNaytto();
    paivitaSumma( model->laskunSumma() );
}


LaskuDialogi::~LaskuDialogi()
{
    kp()->settings()->setValue("LaskuDialogi", saveGeometry());
    delete ui;
    delete model;
    laskuIkkunoita__--;
}

void LaskuDialogi::paivitaSumma(qlonglong summa)
{
    ui->summaLabel->setText( QString("%L1 €").arg(summa / 100.0,0,'f',2) );
}

void LaskuDialogi::esikatsele()
{
    vieMalliin();
    if( !model->tarkastaAlvLukko())
        return;

    if( model->tyyppi() == LaskuModel::RYHMALASKU )
    {
        QByteArray array;
        QBuffer buffer(&array);
        buffer.open(QIODevice::WriteOnly);

        QPdfWriter writer(&buffer);
        writer.setCreator(QString("%1 %2").arg( qApp->applicationName() ).arg( qApp->applicationVersion() ));
        writer.setTitle( tr("Ryhmälaskutus %1").arg(QDateTime::currentDateTime().toString("dd.MM.yyyy hh.mm")) );
        QPainter painter( &writer);

        bool sivunvaihto = false;
        for(const QModelIndex& indeksi : ui->ryhmaView->selectionModel()->selectedRows() )
        {
            if( sivunvaihto )
                writer.newPage();
            model->haeRyhmasta( ryhmaProxy_->mapToSource( indeksi ).row());

            tulostaja->tulosta( &writer, &painter);
            sivunvaihto = true;
        }

        painter.end();
        buffer.close();
        NaytinIkkuna::nayta(array);

    }
    else
    {
        NaytinIkkuna::nayta( tulostaja->pdf() );
    }
}

void LaskuDialogi::finvoice()
{
    vieMalliin();
    if( model->tyyppi() == LaskuModel::RYHMALASKU )
    {
        for(const QModelIndex& indeksi : ui->ryhmaView->selectionModel()->selectedRows() )
        {
            model->haeRyhmasta( ryhmaProxy_->mapToSource( indeksi ).row());
            if( model->verkkolaskuOsoite().isEmpty() || model->verkkolaskuValittaja().isEmpty())
                continue;

            if(Finvoice::muodostaFinvoice(model))
                model->ryhmaModel()->finvoiceMuodostettu( ryhmaProxy_->mapToSource( indeksi).row() );
        }
    }
    else
    {
        if( Finvoice::muodostaFinvoice(model))
        {
            ui->onniLabel->setText( tr("Verkkolasku muodostettu") );
            ui->onniLabel->setStyleSheet("color: green;");
        }
        else
        {
            ui->onniLabel->setText( tr("Verkkolaskun muodostaminen epäonnistui") );
            ui->onniLabel->setStyleSheet("color: red;");
        }
    }
}

void LaskuDialogi::perusteVaihtuu()
{
    int peruste = ui->perusteCombo->currentData().toInt();

    ui->rahaTiliEdit->setVisible( peruste != LaskuModel::MAKSUPERUSTE );
    ui->rahatiliLabel->setVisible( peruste != LaskuModel::MAKSUPERUSTE );

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

}

void LaskuDialogi::haeOsoite()
{
    QSqlQuery kysely;
    QString nimistr = ui->saajaEdit->text();
    nimistr.remove(QRegExp("['\"]"));

    kysely.exec( QString("SELECT json FROM vienti WHERE asiakas='%1' AND iban is null ORDER BY muokattu DESC").arg( nimistr)) ;

    if( kysely.next() )
    {
        JsonKentta json;
        json.fromJson( kysely.value(0).toByteArray() );
        ui->emailEdit->setText( json.str("Email"));
        ui->ytunnus->setText( json.str("YTunnus"));
        ui->verkkoOsoiteEdit->setText( json.str("VerkkolaskuOsoite"));
        ui->verkkoValittajaEdit->setText( json.str("VerkkolaskuValittaja"));

        qDebug() << "Kieli:" << json.str("Kieli");
        if( !json.str("Kieli").isEmpty())
            ui->kieliCombo->setCurrentIndex( ui->kieliCombo->findData( json.str("Kieli") ));

        if( !json.str("Osoite").isEmpty())
        {
            // Haetaan aiempi osoite
            ui->osoiteEdit->setPlainText( json.str("Osoite"));
            return;
        }
    }
    // Osoitetta ei tiedossa, kirjoitetaan nimi
    ui->osoiteEdit->setPlainText( nimistr + "\n" );


}

void LaskuDialogi::vieMalliin()
{
    if( model->laskunSumma() > 0 && ui->perusteCombo->currentData().toInt() != LaskuModel::KATEISLASKU )
        model->asetaErapaiva( ui->eraDate->date());

    model->asetaLisatieto( ui->lisatietoEdit->toPlainText());
    model->asetaOsoite(ui->osoiteEdit->toPlainText());
    model->asetaEmail( ui->emailEdit->text());
    model->asetaToimituspaiva(ui->toimitusDate->date());
    model->asetaLaskunsaajannimi(ui->saajaEdit->text());
    model->asetaKirjausperuste(ui->perusteCombo->currentData().toInt());
    model->asetaAsiakkaanViite(ui->asViiteEdit->text());
    model->asetaVerkkolaskuOsoite(ui->verkkoOsoiteEdit->text());
    model->asetaVerkkolaskuValittaja(ui->verkkoValittajaEdit->text());

    if( ui->ytunnus->hasAcceptableInput())
        model->asetaYTunnus( ui->ytunnus->text());
    else
        model->asetaYTunnus(QString());
}

void LaskuDialogi::accept()
{
    vieMalliin();
    kp()->asetukset()->aseta("LaskuNaytaTuotteet", ui->naytaNappi->isChecked());

    if( model->pvm().isValid() && ( model->pvm() <= kp()->tilitpaatetty() || model->pvm() > kp()->tilikaudet()->kirjanpitoLoppuu() ))
    {
        QMessageBox::critical(this, tr("Kirjanpito on lukittu"),
                              tr("Laskun päivämäärälle %1 ei ole avointa tilikautta. Laskua ei voi tallentaa.").arg( model->pvm().toString("dd.MM.yyyy") ));
        return;
    }

    int rahatilinro = ui->rahaTiliEdit->valittuTilinumero();

    if( model->tallenna(kp()->tilit()->tiliNumerolla( rahatilinro )) )
        QDialog::accept();
    else
        QMessageBox::critical(this, tr("Virhe laskun tallentamisessa"), tr("Laskun tallentaminen epäonnistui"));

}

void LaskuDialogi::rivienKontekstiValikko(QPoint pos)
{
    kontekstiIndeksi=ui->rivitView->indexAt(pos);

    QMenu *menu=new QMenu(this);
    if( kontekstiIndeksi.data(LaskuModel::TuoteKoodiRooli).toInt() )
        menu->addAction(QIcon(":/pic/kitupiikki32.png"), tr("Päivitä tuoteluetteloon"), this, SLOT(paivitaTuoteluetteloon()) );
    else
        menu->addAction(QIcon(":/pic/lisaa.png"), tr("Lisää tuoteluetteloon"), this, SLOT(lisaaTuoteluetteloon()));
    menu->popup(ui->rivitView->viewport()->mapToGlobal(pos));
}

void LaskuDialogi::lisaaTuoteluetteloon()
{
    int tuotekoodi = tuotteet->lisaaTuote( model->rivi( kontekstiIndeksi.row() ) );
    model->setData(kontekstiIndeksi, tuotekoodi, LaskuModel::TuoteKoodiRooli);

    paivitaTuoteluettelonNaytto();
}

void LaskuDialogi::lisaaTuote(const QModelIndex &index)
{
    LaskuRivi rivi = tuotteet->tuote( tuoteProxy->mapToSource(index).row() );
    model->lisaaRivi( rivi);
}

void LaskuDialogi::poistaLaskuRivi()
{
    int indeksi = ui->rivitView->currentIndex().row();
    if( indeksi > -1)
        model->poistaRivi(indeksi);
}

void LaskuDialogi::tuotteidenKonteksiValikko(QPoint pos)
{
    kontekstiIndeksi = tuoteProxy->mapToSource( ui->tuotelistaView->indexAt(pos) );

    QMenu *menu = new QMenu(this);
    menu->addAction(QIcon(":/pic/poistarivi.png"), tr("Poista tuoteluettelosta"), this, SLOT(poistaTuote()));
    menu->popup( ui->tuotelistaView->viewport()->mapToGlobal(pos));
}

void LaskuDialogi::poistaTuote()
{
    tuotteet->poistaTuote( kontekstiIndeksi.row() );
}

void LaskuDialogi::paivitaTuoteluetteloon()
{
    tuotteet->paivitaTuote( model->rivi( kontekstiIndeksi.row() ) );
}

void LaskuDialogi::onkoPostiKaytossa()
{
    // Sähköpostin lähettäminen edellyttää smtp-asetusten laittamista
    ui->spostiNappi->setEnabled( !kp()->settings()->value("SmtpServer").toString().isEmpty()
                                 && ui->emailEdit->text().contains(QRegularExpression(".+@.+\\.\\w+")));
}

void LaskuDialogi::lahetaSahkopostilla()
{

    vieMalliin();
    if( !model->tarkastaAlvLukko())
        return;

    if( model->tyyppi() == LaskuModel::RYHMALASKU)
    {
        ryhmaLahetys_.append(-1);
        if( !ui->ryhmaView->selectionModel()->hasSelection())
            ui->ryhmaView->selectAll();

        for( const QModelIndex& indeksi : ui->ryhmaView->selectionModel()->selectedRows())
        {
            if( !indeksi.data(LaskuRyhmaModel::SahkopostiRooli).toString().isEmpty())
                ryhmaLahetys_.append( ryhmaProxy_->mapToSource(indeksi).row() );
        }
        lahetaRyhmanSeuraava();
        return;
    }


    Smtp *smtp = new Smtp( kp()->settings()->value("SmtpUser").toString(), kp()->settings()->value("SmtpPassword").toString(),
                     kp()->settings()->value("SmtpServer").toString(), kp()->settings()->value("SmtpPort", 465).toInt() );
    connect( smtp, SIGNAL(status(QString)), this, SLOT(smtpViesti(QString)));


    QString kenelta = QString("=?utf-8?Q?%1?= <%2>").arg(kp()->asetukset()->asetus("EmailNimi"))
                                                .arg(kp()->asetukset()->asetus("EmailOsoite"));
    QString kenelle = QString("=?utf-8?Q?%1?= <%2>").arg( ui->saajaEdit->text() )
                                            .arg(ui->emailEdit->text() );

    smtp->lahetaLiitteella(kenelta, kenelle, tr("%3 %1 - %2").arg( model->viitenumero() ).arg( kp()->asetukset()->asetus("Nimi").arg(model->t("laskuotsikko")) ),
                           tulostaja->html(), tr("lasku%1.pdf").arg( model->viitenumero()), tulostaja->pdf(false));


    if( kp()->asetukset()->onko("EmailKopio") )
    {
        // Lähetä kopio myös itsellesi
        Smtp *kopioSmtp = new Smtp( kp()->settings()->value("SmtpUser").toString(), kp()->settings()->value("SmtpPassword").toString(),
                         kp()->settings()->value("SmtpServer").toString(), kp()->settings()->value("SmtpPort", 465).toInt() );
        kopioSmtp->lahetaLiitteella(kenelta, kenelta, tr("Kopio: Lasku %1 - %2").arg( model->viitenumero() ).arg( kp()->asetukset()->asetus("Nimi") ),
                               tulostaja->html(), tr("lasku%1.pdf").arg( model->viitenumero()), tulostaja->pdf(false));
    }

}

void LaskuDialogi::lahetaRyhmanSeuraava(const QString &viesti)
{
    smtpViesti(viesti);

    if( viesti.endsWith('.'))   // Ei ole vielä lopettava viesti
        return;
    else if( viesti == tr("Sähköposti lähetetty"))
        model->ryhmaModel()->sahkopostiLahetetty( ryhmaLahetys_.first() );

    ryhmaLahetys_.removeFirst();
    if( !ryhmaLahetys_.isEmpty() )
    {
        model->haeRyhmasta(ryhmaLahetys_.first());


        Smtp *smtp = new Smtp( kp()->settings()->value("SmtpUser").toString(), kp()->settings()->value("SmtpPassword").toString(),
                         kp()->settings()->value("SmtpServer").toString(), kp()->settings()->value("SmtpPort", 465).toInt() );
        connect( smtp, &Smtp::status, this, &LaskuDialogi::lahetaRyhmanSeuraava);


        QString kenelta = QString("=?utf-8?Q?%1?= <%2>").arg(kp()->asetukset()->asetus("EmailNimi"))
                                                    .arg(kp()->asetukset()->asetus("EmailOsoite"));
        QString kenelle = QString("=?utf-8?Q?%1?= <%2>").arg( model->laskunsaajanNimi() )
                                                .arg(model->email() );

        smtp->lahetaLiitteella(kenelta, kenelle, tr("%3 %1 - %2").arg( model->viitenumero() ).arg( kp()->asetukset()->asetus("Nimi").arg(model->t("laskuotsikko")) ),
                               tulostaja->html(), tr("lasku%1.pdf").arg( model->viitenumero()), tulostaja->pdf(false));

        if( kp()->asetukset()->onko("EmailKopio") )
        {
            // Lähetä kopio myös itsellesi
            Smtp *kopioSmtp = new Smtp( kp()->settings()->value("SmtpUser").toString(), kp()->settings()->value("SmtpPassword").toString(),
                             kp()->settings()->value("SmtpServer").toString(), kp()->settings()->value("SmtpPort", 465).toInt() );
            kopioSmtp->lahetaLiitteella(kenelta, kenelta, tr("Kopio: Lasku %1 - %2").arg( model->viitenumero() ).arg( kp()->asetukset()->asetus("Nimi") ),
                                   tulostaja->html(), tr("lasku%1.pdf").arg( model->viitenumero()), tulostaja->pdf(false));
        }

    }
}

void LaskuDialogi::smtpViesti(const QString &viesti)
{
    ui->onniLabel->setText( viesti );

    if( viesti == tr( "Sähköposti lähetetty" ) )    
        ui->onniLabel->setStyleSheet("color: green;");        
    else if(viesti == tr( "Sähköpostin lähetys epäonnistui" )  )
        ui->onniLabel->setStyleSheet("color: red;");
    else
        ui->onniLabel->setStyleSheet("color: black;");

}

void LaskuDialogi::tulostaLasku()
{
    vieMalliin();
    if( !model->tarkastaAlvLukko())
        return;

    QPageLayout vanhaleiska = kp()->printer()->pageLayout();
    QPageLayout uusileiska = vanhaleiska;
    uusileiska.setUnits(QPageLayout::Millimeter);
    uusileiska.setMargins( QMarginsF(5.0,5.0,5.0,5.0));
    kp()->printer()->setPageLayout(uusileiska);

    QPrintDialog printDialog( kp()->printer(), this );    
    if( printDialog.exec())
    {
        QPainter painter( kp()->printer());
        if( model->tyyppi() == LaskuModel::RYHMALASKU)
        {

            bool sivunvaihto = false;
            for(const QModelIndex& indeksi : ui->ryhmaView->selectionModel()->selectedRows() )
            {
                if( sivunvaihto )
                    kp()->printer()->newPage();
                model->haeRyhmasta( ryhmaProxy_->mapToSource( indeksi).row());

                tulostaja->tulosta( kp()->printer() , &painter);
                sivunvaihto = true;
            }
        }
        else
        {
            tulostaja->tulosta( kp()->printer(), &painter );
        }

        painter.end();
    }

    kp()->printer()->setPageLayout(vanhaleiska);
}

void LaskuDialogi::ryhmaNapit(const QItemSelection &valinta)
{
    ui->tulostaNappi->setEnabled( valinta.size());
    ui->esikatseluNappi->setEnabled( valinta.size());
    ui->verkkolaskuNappi->setEnabled( valinta.size());
    ui->poistaRyhmastaNappi->setEnabled( valinta.size());

    ui->spostiNappi->setEnabled(!kp()->settings()->value("SmtpServer").toString().isEmpty() && valinta.size());
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

    model->ryhmaModel()->lisaa( nimistr, osoite, email, ytunnus, verkkolaskuosoite, verkkolaskuvalittaja);

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

        model->ryhmaModel()->lisaa( dui.nimiEdit->text(), dui.osoiteEdit->toPlainText(), dui.spostiEdit->text(),
                                    dui.YtunnusEdit->text(), dui.verkkolaskuOsoite->text(), dui.valittajaTunnus->text());
    }
}

void LaskuDialogi::tuoAsiakkaitaTiedostosta()
{
    QString tiedostonnimi = QFileDialog::getOpenFileName(this, tr("Tuo laskutettavien luettelo"),
                                                         QString(), tr("csv-tiedostot (*.csv);;Kaikki tiedostot (*)"));
    if( !tiedostonnimi.isEmpty())
    {
        RyhmanTuontiDlg dlg(tiedostonnimi, this);
        if( dlg.exec() == QDialog::Accepted )
            dlg.data()->lisaaLaskuun(model->ryhmaModel());
    }
}

void LaskuDialogi::poistaValitutAsiakkaat()
{
    QList<int> indeksit;
    for(const QModelIndex &indeksi : ui->ryhmaView->selectionModel()->selectedRows())
        indeksit.append( ryhmaProxy_->mapToSource(indeksi).row() );

    // Poistetaan alimmasta alkaen, jotta poistaminen ei sotke
    // rivien numerointia

    qSort( indeksit );
    while( !indeksit.isEmpty())
    {
        model->ryhmaModel()->poista( indeksit.last() );
        indeksit.removeLast();
    }
}

void LaskuDialogi::paivitaRyhmanTallennusNappi()
{
    ui->tallennaNappi->setEnabled( model->ryhmaModel()->rowCount(QModelIndex()) );
}

void LaskuDialogi::verkkolaskuKayttoon()
{
    ui->verkkolaskuNappi->setEnabled( !ui->verkkoOsoiteEdit->text().isEmpty() && !ui->verkkoValittajaEdit->text().isEmpty() );
}

void LaskuDialogi::ytunnusSyotetty(const QString& ytunnus)
{
    bool kelpotunnus =  YTunnusValidator::kelpaako(ytunnus, true);
    ui->tabWidget->setTabEnabled(2, kelpotunnus);
    if( kelpotunnus && ui->verkkoOsoiteEdit->text().isEmpty())
    {
        QString osoite = QString("0037%1").arg(ytunnus);
        osoite.remove('-');
        ui->verkkoOsoiteEdit->setText(osoite);
    }
}


void LaskuDialogi::paivitaTuoteluettelonNaytto()
{
    int tuotteita = tuotteet->rowCount( QModelIndex());
    ui->tuotelistaView->setVisible( tuotteita );
    ui->tuotelistaOhje->setVisible( !tuotteita );
}

int LaskuDialogi::laskuIkkunoita()
{
    return laskuIkkunoita__;
}

void LaskuDialogi::reject()
{
    vieMalliin();
    kp()->asetukset()->aseta("LaskuNaytaTuotteet", ui->naytaNappi->isChecked());

    if( !model->muokattu())
        QDialog::reject();

    else if( QMessageBox::question(this, tr("Hylkää lasku"),
                              tr("Hylkäätkö laskun tallentamatta sitä kirjanpitoon?"))==QMessageBox::Yes)
        QDialog::reject();
}

int LaskuDialogi::laskuIkkunoita__ = 0;
