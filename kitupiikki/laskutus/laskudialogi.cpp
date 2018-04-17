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

#include "db/kirjanpito.h"

#include "laskudialogi.h"
#include "ui_laskudialogi.h"
#include "laskuntulostaja.h"
#include "laskutusverodelegaatti.h"

#include "kirjaus/eurodelegaatti.h"
#include "kirjaus/kohdennusdelegaatti.h"
#include "kirjaus/tilidelegaatti.h"

#include "kirjaus/verodialogi.h"
#include "tools/pdfikkuna.h"

LaskuDialogi::LaskuDialogi(QWidget *parent, AvoinLasku hyvitettavaLasku) :
    QDialog(parent),
    ui(new Ui::LaskuDialogi)
{
    setAttribute(Qt::WA_DeleteOnClose);

    ui->setupUi(this);

    ui->perusteCombo->addItem(QIcon(":/pic/suorite.png"), tr("Suoriteperusteinen"), LaskuModel::SUORITEPERUSTE);
    ui->perusteCombo->addItem(QIcon(":/pic/kirje.png"), tr("Laskutusperusteinen"), LaskuModel::LASKUTUSPERUSTE);
    ui->perusteCombo->addItem(QIcon(":/pic/euro.png"), tr("Maksuperusteinen"), LaskuModel::MAKSUPERUSTE);
    ui->perusteCombo->addItem(QIcon(":/pic/kateinen.png"), tr("Käteiskuitti"), LaskuModel::KATEISLASKU);

    model = new LaskuModel(this, hyvitettavaLasku);

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
    // Hyvityslaskun asetukset
    if( !hyvitettavaLasku.viite.isEmpty())
    {
        setWindowTitle( tr("Hyvityslasku"));
        ui->perusteCombo->setCurrentIndex( hyvitettavaLasku.json.luku("Kirjausperuste") );
        ui->perusteCombo->setEnabled(false);
        ui->saajaEdit->setText( hyvitettavaLasku.asiakas );
        ui->osoiteEdit->setPlainText( hyvitettavaLasku.json.str("Osoite"));
        ui->emailEdit->setText( hyvitettavaLasku.json.str("Email"));
        ui->eraDate->setDate( hyvitettavaLasku.erapvm );
        ui->eraDate->setEnabled(false);
        ui->toimitusDate->setDate( hyvitettavaLasku.json.date("Toimituspvm"));
        ui->toimitusDate->setEnabled(false);
        ui->rahaTiliEdit->valitseTiliNumerolla( hyvitettavaLasku.json.luku("Saatavatili") );
        ui->rahaTiliEdit->setEnabled(false);

        ui->lisatietoEdit->setPlainText( tr("Hyvityslasku laskulle %1, päiväys %2")
                                         .arg(hyvitettavaLasku.viite)
                                         .arg(hyvitettavaLasku.pvm.toString("dd.MM.yyyy")));
    }
    else
    {
        ui->eraDate->setMinimumDate( kp()->paivamaara() );
        ui->perusteCombo->setCurrentIndex( ui->perusteCombo->findData( kp()->asetukset()->luku("LaskuKirjausperuste") ));
        perusteVaihtuu();

        ui->toimitusDate->setDate( kp()->paivamaara() );
        ui->eraDate->setDate( kp()->paivamaara().addDays( kp()->asetukset()->luku("LaskuMaksuaika")));
    }

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
    connect( model, SIGNAL(summaMuuttunut(int)), this, SLOT(paivitaSumma(int)));
    connect( ui->perusteCombo, SIGNAL(currentIndexChanged(int)), this, SLOT(perusteVaihtuu()));
    connect( ui->saajaEdit, SIGNAL(textChanged(QString)), this, SLOT(haeOsoite()));
    connect( ui->rivitView, SIGNAL(customContextMenuRequested(QPoint)), this, SLOT(rivienKontekstiValikko(QPoint)));

    connect( ui->ohjeNappi, &QPushButton::clicked, [] { kp()->ohje("laskutus#uusi-lasku");});

    connect( ui->tuotelistaView, SIGNAL(clicked(QModelIndex)), this, SLOT(lisaaTuote(QModelIndex)));
    connect( ui->tuotelistaView, SIGNAL(customContextMenuRequested(QPoint)), this, SLOT(tuotteidenKonteksiValikko(QPoint)));
    connect( ui->emailEdit, SIGNAL(textChanged(QString)), this, SLOT(onkoPostiKaytossa()));



    ui->rivitView->horizontalHeader()->setSectionResizeMode(LaskuModel::NIMIKE, QHeaderView::Stretch);
    ui->tuotelistaView->horizontalHeader()->setSectionResizeMode(TuoteModel::NIMIKE, QHeaderView::Stretch);

    tulostaja = new LaskunTulostaja(model);

    paivitaTuoteluettelonNaytto();
}

LaskuDialogi::~LaskuDialogi()
{
    kp()->asetukset()->aseta("LaskuNaytaTuotteet", ui->naytaNappi->isChecked());
    delete ui;
}

void LaskuDialogi::paivitaSumma(int summa)
{
    ui->summaLabel->setText( QString("%L1 €").arg(summa / 100.0,0,'f',2) );
}

void LaskuDialogi::esikatsele()
{
    vieMalliin();

    PdfIkkuna::naytaPdf( tulostaja->pdf() );
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

    kysely.exec( QString("SELECT json FROM vienti WHERE asiakas='%1' ORDER BY muokattu DESC").arg( nimistr)) ;

    if( kysely.next() )
    {
        JsonKentta json;
        json.fromJson( kysely.value(0).toByteArray() );
        ui->emailEdit->setText( json.str("Email"));

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
    if( model->laskunSumma() > 0 && ui->perusteCombo->currentData().toInt() != LaskuModel::KATEISLASKU)
        model->asetaErapaiva( ui->eraDate->date());

    model->asetaLisatieto( ui->lisatietoEdit->toPlainText());
    model->asetaOsoite(ui->osoiteEdit->toPlainText());
    model->asetaEmail( ui->emailEdit->text());
    model->asetaToimituspaiva(ui->toimitusDate->date());
    model->asetaLaskunsaajannimi(ui->saajaEdit->text());
    model->asetaKirjausperuste(ui->perusteCombo->currentData().toInt());
}

void LaskuDialogi::accept()
{
    vieMalliin();
    if( model->tallenna(kp()->tilit()->tiliNumerolla( ui->rahaTiliEdit->valittuTilinumero() )) )
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

    // Luo tilapäisen pdf-tiedoston
    QString tpnimi = kp()->tilapainen("lasku-XXXX.pdf");
    QFile tiedosto(tpnimi);
    tiedosto.open(QIODevice::WriteOnly);
    tiedosto.write( tulostaja->pdf() );
    tiedosto.close();

    QString kenelta = QString("\"%1\" <%2>").arg(kp()->asetukset()->asetus("EmailNimi"))
                                                .arg(kp()->asetukset()->asetus("EmailOsoite"));
    QString kenelle = QString("\"%1\" <%2>").arg( ui->saajaEdit->text() )
                                            .arg(ui->emailEdit->text() );

    QStringList lista;
    lista << tpnimi;
    smtpViesti("Lähetetään sähköpostilla...");
    smtp->sendMail(kenelta, kenelle, tr("Lasku"), tulostaja->html(), lista);

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
        tulostaja->tulosta( kp()->printer() );
    }
}

void LaskuDialogi::paivitaTuoteluettelonNaytto()
{
    int tuotteita = tuotteet->rowCount( QModelIndex());
    ui->tuotelistaView->setVisible( tuotteita );
    ui->tuotelistaOhje->setVisible( !tuotteita );
}

void LaskuDialogi::reject()
{
    if( QMessageBox::question(this, tr("Hylkää lasku"),
                              tr("Hylkäätkö laskun tallentamatta sitä kirjanpitoon?"))==QMessageBox::Yes)
        QDialog::reject();
}
