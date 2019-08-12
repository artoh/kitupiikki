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
#include "naytin/naytinview.h"
#include "validator/ytunnusvalidator.h"
#include "asiakkaatmodel.h"
#include "ryhmaasiakasproxy.h"

#include "finvoice.h"

#include "ui_yhteystiedot.h"

#include "validator/ytunnusvalidator.h"

#include "laskurivitmodel.h"

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

    ui->kieliCombo->addItem(QIcon(":/pic/fi.png"), tr("Suomi"), "FI");
    ui->kieliCombo->addItem(QIcon(":/pic/se.png"), tr("Ruotsi"), "SE");
    ui->kieliCombo->addItem(QIcon(":/pic/en.png"), tr("Englanti"), "EN");

    model->lisaaRivi();

    tuotteet = kp()->tuotteet();
    tuoteProxy = new QSortFilterProxyModel(this);
    tuoteProxy->setSourceModel(tuotteet);
    tuoteProxy->setSortLocaleAware(true);

//    ui->nroLabel->setText( QString::number(model->laskunro()));
//    ui->ytunnus->setValidator( new YTunnusValidator(true));

    ui->asiakas->alusta(false);



    ui->eraDate->setDate( model->erapaiva() );
    ui->eraDate->setEnabled( model->tyyppi() != LaskuModel::HYVITYSLASKU );
    ui->toimitusDate->setDate( model->toimituspaiva());
    ui->lisatietoEdit->setPlainText( model->lisatieto() );
    ui->asViiteEdit->setText( model->asiakkaanViite());
    ui->viivkorkoSpin->setValue( model->viivastysKorko() );

    ui->tallennaNappi->setEnabled( model->muokattu() );

    if( model->tyyppi() == LaskuModel::HYVITYSLASKU)
    {
        setWindowTitle( tr("Hyvityslasku"));
        ui->toimituspvmLabel->setText( tr("Hyvityspvm"));
        connect( ui->ohjeNappi, &QPushButton::clicked, [] { kp()->ohje("laskutus/uusi#hyvityslasku");});
    }
    else if( model->tyyppi() == LaskuModel::MAKSUMUISTUTUS)
    {
        setWindowTitle( tr("Maksumuistutus"));
        connect( ui->ohjeNappi, &QPushButton::clicked, [] { kp()->ohje("laskutus/muistutus");});

    }
    else if( model->tyyppi() == LaskuModel::LASKU || model->tyyppi() == LaskuModel::RYHMALASKU)
    {
        perusteVaihtuu();

        if( model->tyyppi() == LaskuModel::LASKU)
            connect( ui->ohjeNappi, &QPushButton::clicked, [] { kp()->ohje("laskutus/uusi");});
        else
            connect( ui->ohjeNappi, &QPushButton::clicked, [] { kp()->ohje("laskutus/ryhma");});
    }

    if( model->tyyppi() == LaskuModel::RYHMALASKU)
    {

        setWindowTitle("Ryhmän laskutus");

        ryhmaProxy_ = new QSortFilterProxyModel(this);
        ryhmaProxy_->setSourceModel( model->ryhmaModel());

        AsiakkaatModel *asiakkaat = new AsiakkaatModel(model);
        asiakkaat->paivita();

        // Asiakasluettelo suodatetaan ryhmäproxyn läpi
        RyhmaAsiakasProxy *ryhmaAsiakasProxy = new RyhmaAsiakasProxy(this);
        ryhmaAsiakasProxy->asetaRyhmaModel( model->ryhmaModel() );
        ryhmaAsiakasProxy->setSourceModel(asiakkaat);
        ryhmaAsiakasProxy->setFilterCaseSensitivity(Qt::CaseInsensitive);


//        ui->saajaEdit->hide();
        ui->osoiteEdit->hide();
//        ui->emailEdit->hide();
//        ui->ytunnus->hide();
//        ui->viiteLabel->hide();
//        ui->nroLabel->hide();
        ui->asViiteEdit->hide();

        ui->esikatseluNappi->setEnabled(false);
        ui->tallennaNappi->setEnabled(false);


        connect( ui->ohjeNappi, &QPushButton::clicked, [] { kp()->ohje("laskutus/ryhma");});

        connect( model->ryhmaModel(), &LaskuRyhmaModel::rowsInserted, this, &LaskuDialogi::paivitaRyhmanTallennusNappi);
        connect( model->ryhmaModel(), &LaskuRyhmaModel::rowsRemoved, this, &LaskuDialogi::paivitaRyhmanTallennusNappi);

    }
    else
    {
        ui->tabWidget->removeTab(RYHMAT);
        if( !kp()->asetukset()->onko("VerkkolaskuKaytossa"))
            ui->tabWidget->removeTab(VERKKOLASKU);

    }

    connect( ui->esikatseluNappi, SIGNAL(clicked(bool)), this, SLOT(esikatselu()));

    connect( model, &LaskuModel::summaMuuttunut, this, &LaskuDialogi::paivitaSumma);
    connect( ui->kieliCombo, &QComboBox::currentTextChanged, [this]() { this->model->asetaKieli( this->ui->kieliCombo->currentData().toString()); } );

    ui->kieliCombo->setCurrentIndex( ui->kieliCombo->findData( model->kieli() ) );


    connect( ui->toimitusDate, SIGNAL(dateChanged(QDate)), model, SLOT(asetaToimituspaiva(QDate)));
    connect( model, &LaskuModel::laskuaMuokattu, ui->tallennaNappi, &QPushButton::setEnabled );

    connect( ui->eraDate, &KpDateEdit::dateChanged, model, &LaskuModel::asetaErapaiva);
    connect( ui->lisatietoEdit, &QPlainTextEdit::textChanged, this, &LaskuDialogi::vieMalliin);
    connect( ui->osoiteEdit, &QPlainTextEdit::textChanged, this, &LaskuDialogi::vieMalliin );
    connect( ui->toimitusDate, &KpDateEdit::dateChanged, model, &LaskuModel::asetaToimituspaiva);
    connect( ui->email, &QLineEdit::textChanged, model, &LaskuModel::asetaEmail);
    connect( ui->asViiteEdit, &QLineEdit::textChanged, model, &LaskuModel::asetaAsiakkaanViite);
    connect( ui->viivkorkoSpin, SIGNAL( valueChanged(double) ), model, SLOT(asetaViivastyskorko(double)) );



    paivitaSumma( model->laskunSumma() );

    // UUTTA
    lisaaRiviTab();
    connect( ui->asiakas, &AsiakasToimittajaValinta::valittu, this, &LaskuDialogi::asiakasValittu);
    connect( ui->email, &QLineEdit::textChanged, this, &LaskuDialogi::paivitaLaskutustavat);
    connect( ui->laskutusCombo, &QComboBox::currentTextChanged, this, &LaskuDialogi::laskutusTapaMuuttui);

    paivitaLaskutustavat();
    ui->jaksoDate->setNull();
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

void LaskuDialogi::esikatselu()
{
    vieMalliin();
    if( !model->tarkastaAlvLukko())
        return;

    esikatsele();
}

void LaskuDialogi::tulosta(QPagedPaintDevice *printer) const
{
    QPainter painter( printer);
    {
        tulostaja->tulosta( printer, &painter );
    }

    painter.end();
}

QString LaskuDialogi::otsikko() const
{
    return tr("Lasku %1").arg(model->laskunro());
}

void LaskuDialogi::finvoice()
{

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

void LaskuDialogi::haeOsoite()
{
/*    QSqlQuery kysely;
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
    vieMalliin(); */
}

void LaskuDialogi::vieMalliin()
{ /*
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
    model->asetaViivastyskorko( ui->viivkorkoSpin->value() );

    if( ui->ytunnus->hasAcceptableInput())
        model->asetaYTunnus( ui->ytunnus->text());
    else
        model->asetaYTunnus(QString()); */
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

/*    int rahatilinro = ui->rahaTiliEdit->valittuTilinumero();

    if( model->tallenna(kp()->tilit()->tiliNumerolla( rahatilinro )) )
        QDialog::accept();
    else
        QMessageBox::critical(this, tr("Virhe laskun tallentamisessa"), tr("Laskun tallentaminen epäonnistui"));
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

void LaskuDialogi::lisaaTuoteluetteloon()
{
    int tuotekoodi = tuotteet->lisaaTuote( model->rivi( kontekstiIndeksi.row() ) );
    model->setData(kontekstiIndeksi, tuotekoodi, LaskuModel::TuoteKoodiRooli);

}

void LaskuDialogi::lisaaTuote(const QModelIndex &index)
{
    LaskuRivi rivi = tuotteet->tuote( tuoteProxy->mapToSource(index).row() );
    model->lisaaRivi( rivi);
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
//    ui->spostiNappi->setEnabled( !kp()->settings()->value("SmtpServer").toString().isEmpty()
//                                 && ui->emailEdit->text().contains(QRegularExpression(".+@.+\\.\\w+")));
}

void LaskuDialogi::lahetaSahkopostilla()
{

    vieMalliin();
    if( !model->tarkastaAlvLukko())
        return;



    Smtp *smtp = new Smtp( kp()->settings()->value("SmtpUser").toString(), kp()->settings()->value("SmtpPassword").toString(),
                     kp()->settings()->value("SmtpServer").toString(), kp()->settings()->value("SmtpPort", 465).toInt() );
    connect( smtp, SIGNAL(status(QString)), this, SLOT(smtpViesti(QString)));


    QString kenelta = QString("=?utf-8?B?%1?= <%2>").arg( QString(kp()->asetukset()->asetus("EmailNimi").toUtf8().toBase64())  )
                                                .arg(kp()->asetukset()->asetus("EmailOsoite"));
//    QString kenelle = QString("=?utf-8?B?%1?= <%2>").arg( QString( ui->saajaEdit->text().toUtf8().toBase64()) )
//                                            .arg(ui->emailEdit->text() );

//    smtp->lahetaLiitteella(kenelta, kenelle, tr("%3 %1 - %2").arg( model->viitenumero() ).arg( kp()->asetukset()->asetus("Nimi") ).arg(model->t("laskuotsikko")) ,
//                           tulostaja->html(), tr("lasku%1.pdf").arg( model->viitenumero()), tulostaja->pdf(false));


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


        QString kenelta = QString("=?utf-8?B?%1?= <%2>").arg( QString(kp()->asetukset()->asetus("EmailNimi").toUtf8().toBase64()) )
                                                    .arg(kp()->asetukset()->asetus("EmailOsoite"));
        QString kenelle = QString("=?utf-8?B?%1?= <%2>").arg( QString( model->laskunsaajanNimi().toUtf8().toBase64()) )
                                                .arg(model->email() );

        smtp->lahetaLiitteella(kenelta, kenelle, tr("%3 %1 - %2").arg( model->viitenumero() ).arg( kp()->asetukset()->asetus("Nimi")).arg(model->t("laskuotsikko") ),
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
    ui->tallennaNappi->setEnabled( model->ryhmaModel()->rowCount(QModelIndex()) );
}

void LaskuDialogi::verkkolaskuKayttoon()
{
//    ui->verkkolaskuNappi->setEnabled( !ui->verkkoOsoiteEdit->text().isEmpty() && !ui->verkkoValittajaEdit->text().isEmpty() );
}

void LaskuDialogi::asiakasValittu(int asiakasId)
{
    KpKysely *kysely = kpk( QString("/asiakkaat/%1").arg(asiakasId) );
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

    // TODO: Mahdolliset toimitustavat
}

void LaskuDialogi::paivitaLaskutustavat()
{
    int nykyinen = ui->laskutusCombo->currentData().toInt();
    ui->laskutusCombo->clear();

    ui->laskutusCombo->addItem( QIcon(":/pic/tulosta.png"), tr("Tulosta lasku"), LaskuModel::TULOSTA);

    QRegularExpression emailRe(R"(^([\w-]*(\.[\w-]+)?)+@(\w+\.\w+)(\.\w+)*$)");
    if( emailRe.match( ui->email->text()).hasMatch() )
            ui->laskutusCombo->addItem(QIcon(":/pic/email.png"), tr("Lähetä sähköpostilla"), LaskuModel::SAHKOPOSTI);

    ui->laskutusCombo->addItem( QIcon(":/pic/kateinen.png"), tr("Tulosta käteiskuitti"), LaskuModel::KATEISLASKU);

    ui->laskutusCombo->setCurrentIndex(  ui->laskutusCombo->findData(nykyinen) );
    if( ui->laskutusCombo->currentIndex() < 0)
        ui->laskutusCombo->setCurrentIndex(0);
}

void LaskuDialogi::laskutusTapaMuuttui()
{
    int laskutustapa = ui->laskutusCombo->currentData().toInt();
    if( laskutustapa == LaskuModel::SAHKOPOSTI)
    {
        ui->valmisNappi->setText( tr("Tallenna ja lähetä sähköpostilla"));
        ui->valmisNappi->setIcon(QIcon(":/pic/email.png"));
    } else {
        ui->valmisNappi->setText( tr("Tallenna ja tulosta"));
        ui->valmisNappi->setIcon(QIcon(":/pic/email.png"));
    }

    ui->eraLabel->setVisible( laskutustapa != LaskuModel::KATEISLASKU );
    ui->eraDate->setVisible( laskutustapa != LaskuModel::KATEISLASKU );
    ui->viivkorkoLabel->setVisible( laskutustapa != LaskuModel::KATEISLASKU );
    ui->viivkorkoSpin->setVisible( laskutustapa != LaskuModel::KATEISLASKU );
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

    LaskuRivitModel *lrivit = new LaskuRivitModel(this);

    rivitView->setModel(lrivit);

    rivitView->horizontalHeader()->setSectionResizeMode(LaskuModel::NIMIKE, QHeaderView::Stretch);
    rivitView->setItemDelegateForColumn(LaskuModel::AHINTA, new EuroDelegaatti());
    rivitView->setItemDelegateForColumn(LaskuModel::TILI, new TiliDelegaatti());
    rivitView->setSelectionMode(QTableView::SingleSelection);


    KohdennusDelegaatti *kohdennusDelegaatti = new KohdennusDelegaatti();
    rivitView->setItemDelegateForColumn(LaskuModel::KOHDENNUS, kohdennusDelegaatti );

    connect( ui->toimitusDate , SIGNAL(dateChanged(QDate)), kohdennusDelegaatti, SLOT(asetaKohdennusPaiva(QDate)));
    connect( tuoteFiltterinEditori, &QLineEdit::textChanged, proxy, &QSortFilterProxyModel::setFilterFixedString);


    rivitView->setItemDelegateForColumn(LaskuModel::BRUTTOSUMMA, new EuroDelegaatti());
    rivitView->setItemDelegateForColumn(LaskuModel::ALV, new LaskutusVeroDelegaatti());

    rivitView->setColumnHidden( LaskuModel::ALV, !kp()->asetukset()->onko("AlvVelvollinen") );
    rivitView->setColumnHidden( LaskuModel::KOHDENNUS, !kp()->kohdennukset()->kohdennuksia());

    split->setStretchFactor(0,1);
    split->setStretchFactor(1,3);

    ui->tabWidget->insertTab(0, split, QIcon(":/pic/vientilista.png"),"Rivit");
    ui->tabWidget->setCurrentWidget(split);

    connect( tuoteView, &QTableView::clicked, [lrivit, proxy, tuoteModel] (const QModelIndex& index)
        { lrivit->lisaaTuote( tuoteModel->tuoteMap( proxy->mapToSource(index).row()) ); }  );

}

int LaskuDialogi::laskuIkkunoita()
{
    return laskuIkkunoita__;
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

int LaskuDialogi::laskuIkkunoita__ = 0;
