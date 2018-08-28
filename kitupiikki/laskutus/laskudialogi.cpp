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

#include "kirjaus/eurodelegaatti.h"
#include "kirjaus/kohdennusdelegaatti.h"
#include "kirjaus/tilidelegaatti.h"

#include "kirjaus/verodialogi.h"
#include "naytin/naytinikkuna.h"
#include "validator/ytunnusvalidator.h"
#include "asiakkaatmodel.h"

#include <QDebug>

#include <QPrinter>
#include <QPrintDialog>
#include <QDesktopServices>
#include <QCompleter>
#include <QSqlQueryModel>
#include <QSqlQuery>
#include <QRegExp>
#include <QMessageBox>

#include <QMenu>
#include <QAction>

#include <QSettings>
#include <QRegularExpression>

#include <QMessageBox>
#include <QPdfWriter>


LaskuDialogi::LaskuDialogi(LaskuModel *laskumodel) :
    model(laskumodel), ui( new Ui::LaskuDialogi)
{
    if( !laskumodel)
        model = new LaskuModel();

    setAttribute(Qt::WA_DeleteOnClose);

    ui->setupUi(this);

    ui->perusteCombo->addItem(QIcon(":/pic/suorite.png"), tr("Suoriteperusteinen"), LaskuModel::SUORITEPERUSTE);
    ui->perusteCombo->addItem(QIcon(":/pic/kirje.png"), tr("Laskutusperusteinen"), LaskuModel::LASKUTUSPERUSTE);
    ui->perusteCombo->addItem(QIcon(":/pic/euro.png"), tr("Maksuperusteinen"), LaskuModel::MAKSUPERUSTE);
    ui->perusteCombo->addItem(QIcon(":/pic/kateinen.png"), tr("Käteiskuitti"), LaskuModel::KATEISLASKU);

    connect( ui->toimitusDate, SIGNAL(dateChanged(QDate)), model, SLOT(asetaToimituspaiva(QDate)));
    connect(ui->eraDate, SIGNAL(dateChanged(QDate)), model, SLOT(asetaErapaiva(QDate)));

    model->lisaaRivi();

    tuotteet = kp()->tuotteet();
    tuoteProxy = new QSortFilterProxyModel(this);
    tuoteProxy->setSourceModel(tuotteet);
    ui->tuotelistaView->setModel(tuoteProxy);
    ui->tuotelistaView->sortByColumn(TuoteModel::NIMIKE, Qt::AscendingOrder);
    tuoteProxy->setSortLocaleAware(true);
    connect( ui->tuotehakuEdit, SIGNAL(textChanged(QString)), tuoteProxy, SLOT(setFilterFixedString(QString)) );

    ui->nroLabel->setText( QString::number(model->laskunro()));
    ui->ytunnus->setValidator( new YTunnusValidator());

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


    // Laitetaan täydentäjä nimen syöttöön
    QCompleter *nimiTaydentaja = new QCompleter(this);
    QSqlQueryModel *sqlmalli = new QSqlQueryModel(this);
    sqlmalli->setQuery("select distinct asiakas from vienti order by asiakas");
    nimiTaydentaja->setModel(sqlmalli);
    nimiTaydentaja->setModelSorting(QCompleter::CaseSensitivelySortedModel);
    ui->saajaEdit->setCompleter(nimiTaydentaja);

    connect( ui->lisaaNappi, SIGNAL(clicked(bool)), model, SLOT(lisaaRivi()));
    connect( ui->poistaNappi, SIGNAL(clicked(bool)), this, SLOT(poistaLaskuRivi()));
    connect( ui->tulostaNappi, SIGNAL(clicked(bool)), this, SLOT(tulostaLasku()));
    connect( ui->esikatseluNappi, SIGNAL(clicked(bool)), this, SLOT(esikatsele()));
    connect( ui->spostiNappi, SIGNAL(clicked(bool)), this, SLOT(lahetaSahkopostilla()));

    connect( model, &LaskuModel::summaMuuttunut, this, &LaskuDialogi::paivitaSumma);
    connect( ui->perusteCombo, SIGNAL(currentIndexChanged(int)), this, SLOT(perusteVaihtuu()));
    connect( ui->saajaEdit, SIGNAL(textChanged(QString)), this, SLOT(haeOsoite()));
    connect( ui->rivitView, SIGNAL(customContextMenuRequested(QPoint)), this, SLOT(rivienKontekstiValikko(QPoint)));

    connect( ui->ohjeNappi, &QPushButton::clicked, [] { kp()->ohje("laskutus#uusi-lasku");});

    connect( ui->tuotelistaView, SIGNAL(clicked(QModelIndex)), this, SLOT(lisaaTuote(QModelIndex)));
    connect( ui->tuotelistaView, SIGNAL(customContextMenuRequested(QPoint)), this, SLOT(tuotteidenKonteksiValikko(QPoint)));
    connect( ui->emailEdit, SIGNAL(textChanged(QString)), this, SLOT(onkoPostiKaytossa()));

    connect( ui->asiakasLista, &QListView::clicked, this, &LaskuDialogi::lisaaAsiakasListalta);

    ui->rivitView->horizontalHeader()->setSectionResizeMode(LaskuModel::NIMIKE, QHeaderView::Stretch);
    ui->tuotelistaView->horizontalHeader()->setSectionResizeMode(TuoteModel::NIMIKE, QHeaderView::Stretch);
    ui->ryhmaView->horizontalHeader()->setStretchLastSection(true);
    ui->ryhmaView->setSelectionBehavior(QTableView::SelectRows);

    tulostaja = new LaskunTulostaja(model);

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


    if( model->tyyppi() == LaskuModel::HYVITYSLASKU)
    {
        setWindowTitle( tr("Hyvityslasku"));
        ui->toimituspvmLabel->setText( tr("Hyvityspvm"));
        ui->rahaTiliEdit->valitseTiliIdlla( model->viittausLasku().tiliid );
        ui->rahaTiliEdit->setEnabled(false);
    }
    else if( model->tyyppi() == LaskuModel::MAKSUMUISTUTUS)
    {
        setWindowTitle( tr("Maksumuistutus"));
        ui->rahaTiliEdit->valitseTiliIdlla( model->viittausLasku().tiliid );
        ui->rahaTiliEdit->setEnabled(false);

    }
    else if( model->tyyppi() == LaskuModel::LASKU)
    {
        ui->eraDate->setMinimumDate( kp()->paivamaara() );
        perusteVaihtuu();

        ui->eraDate->setDate( kp()->paivamaara().addDays( kp()->asetukset()->luku("LaskuMaksuaika")));
    }

    if( model->tyyppi() == LaskuModel::RYHMALASKU)
    {
        setWindowTitle("Ryhmän laskutus");

        ui->ryhmaView->setModel( model->ryhmaModel() );
        AsiakkaatModel *asiakkaat = new AsiakkaatModel(model);
        asiakkaat->paivita();

        ui->asiakasLista->setModel( asiakkaat );

        ui->saajaEdit->hide();
        ui->osoiteEdit->hide();
        ui->emailEdit->hide();
        ui->ytunnus->hide();
        ui->viiteLabel->hide();
        ui->nroLabel->hide();

    }
    else
    {
        ui->tabWidget->removeTab(2);
    }

    paivitaTuoteluettelonNaytto();
    paivitaSumma( model->laskunSumma() );
}


LaskuDialogi::~LaskuDialogi()
{
    kp()->asetukset()->aseta("LaskuNaytaTuotteet", ui->naytaNappi->isChecked());
    delete ui;
    delete model;
}

void LaskuDialogi::paivitaSumma(qlonglong summa)
{
    ui->summaLabel->setText( QString("%L1 €").arg(summa / 100.0,0,'f',2) );
}

void LaskuDialogi::esikatsele()
{
    if( model->tyyppi() == LaskuModel::RYHMALASKU )
    {
        QByteArray array;
        QBuffer buffer(&array);
        buffer.open(QIODevice::WriteOnly);

        QPdfWriter writer(&buffer);
        writer.setCreator(QString("%1 %2").arg( qApp->applicationName() ).arg( qApp->applicationVersion() ));
        writer.setTitle( tr("Ryhmälaskutus %1").arg(QDateTime::currentDateTime().toString("dd.MM.yyyy hh.mm")) );
        QPainter painter( &writer);

        // Nyt ensiksi koko poppoo
        for(int i=0; i < model->ryhmaModel()->rowCount(QModelIndex()); i++)
        {
            if(i)
                writer.newPage();
            model->haeRyhmasta(i);
            tulostaja->tulosta( &writer, &painter);
        }
        painter.end();
        buffer.close();
        NaytinIkkuna::nayta(array);

    }
    else
    {
        vieMalliin();
        NaytinIkkuna::nayta( tulostaja->pdf() );
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

    if( ui->ytunnus->hasAcceptableInput())
        model->asetaYTunnus( ui->ytunnus->text());
    else
        model->asetaYTunnus(QString());
}

void LaskuDialogi::accept()
{
    vieMalliin();

    if( model->pvm().isValid() && ( model->pvm() <= kp()->tilitpaatetty() || model->pvm() > kp()->tilikaudet()->kirjanpitoLoppuu() ))
    {
        QMessageBox::critical(this, tr("Kirjanpito on lukittu"),
                              tr("Laskun päivämäärälle %1 ei ole avointa tilikautta. Laskua ei voi tallentaa.").arg( model->pvm().toString("dd.MM.yyyy") ));
        return;
    }

    int rahatilinro = ui->rahaTiliEdit->valittuTilinumero();

    if( model->tallenna(kp()->tilit()->tiliNumerolla( rahatilinro ) ) )
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
    QSettings settings;
    ui->spostiNappi->setEnabled( !settings.value("SmtpServer").toString().isEmpty() && ui->emailEdit->text().contains(QRegularExpression(".+@.+\\.\\w+")));
}

void LaskuDialogi::lahetaSahkopostilla()
{
    vieMalliin();
    QSettings settings;

    Smtp *smtp = new Smtp( settings.value("SmtpUser").toString(), settings.value("SmtpPassword").toString(),
                     settings.value("SmtpServer").toString(), settings.value("SmtpPort", 465).toInt() );
    connect( smtp, SIGNAL(status(QString)), this, SLOT(smtpViesti(QString)));


    QString kenelta = QString("=?utf-8?Q?%1?= <%2>").arg(kp()->asetukset()->asetus("EmailNimi"))
                                                .arg(kp()->asetukset()->asetus("EmailOsoite"));
    QString kenelle = QString("=?utf-8?Q?%1?= <%2>").arg( ui->saajaEdit->text() )
                                            .arg(ui->emailEdit->text() );

    smtp->lahetaLiitteella(kenelta, kenelle, tr("Lasku %1 - %2").arg( model->viitenumero() ).arg( kp()->asetukset()->asetus("Nimi") ),
                           tulostaja->html(), tr("lasku%1.pdf").arg( model->viitenumero()), tulostaja->pdf());

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
    QPrintDialog printDialog( kp()->printer(), this );
    if( printDialog.exec())
    {
        QPainter painter( kp()->printer());
        tulostaja->tulosta( kp()->printer(), &painter );
    }
}

void LaskuDialogi::lisaaAsiakasListalta(const QModelIndex &indeksi)
{
    // Tässä testivaiheessa lisätään vain asiakkaan nimi
    model->ryhmaModel()->lisaa( indeksi.data(AsiakkaatModel::NimiRooli).toString(),"Osoite","asiakas@email" );
}

void LaskuDialogi::paivitaTuoteluettelonNaytto()
{
    int tuotteita = tuotteet->rowCount( QModelIndex());
    ui->tuotelistaView->setVisible( tuotteita );
    ui->tuotelistaOhje->setVisible( !tuotteita );
}

void LaskuDialogi::reject()
{
    vieMalliin();

    if( !model->muokattu())
        QDialog::reject();

    else if( QMessageBox::question(this, tr("Hylkää lasku"),
                              tr("Hylkäätkö laskun tallentamatta sitä kirjanpitoon?"))==QMessageBox::Yes)
        QDialog::reject();
}

