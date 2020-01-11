/*
   Copyright (C) 2019 Arto Hyvättinen

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
#include "vanhatuontidlg.h"
#include "ui_vanhatuontidlg.h"
#include <QSettings>
#include <QFile>
#include <QListWidgetItem>

#include <QDebug>
#include <QSqlError>
#include <QSqlQuery>
#include <QFileDialog>
#include <QFileInfo>
#include <QDir>
#include <QMessageBox>
#include <QRegularExpressionValidator>
#include <QJsonDocument>
#include <QDate>

#include "uusikirjanpito/uusivelho.h"
#include "db/kielikentta.h"
#include "db/kirjanpito.h"
#include "sqlite/sqlitemodel.h"
#include "db/tositetyyppimodel.h"

#include "model/tosite.h"
#include "model/tositeviennit.h"
#include "model/tositevienti.h"

VanhatuontiDlg::VanhatuontiDlg(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::VanhatuontiDlg)
{
    ui->setupUi(this);
    ui->tuoNappi->setVisible(false);
    ui->jatkaNappi->setVisible(false);
    alustaTuonti();

    connect( ui->valitseTiedosto, &QPushButton::clicked, this, &VanhatuontiDlg::tuoTiedostosta);
    connect( ui->lista, &QListWidget::itemClicked, [this] (QListWidgetItem *item) { this->avaaTietokanta( item->data(Qt::UserRole).toString()) ;});
    connect( ui->jatkaNappi, &QPushButton::clicked, this, &VanhatuontiDlg::alustaSijainti);
    connect( ui->hakemistoNappi, &QPushButton::clicked, this, &VanhatuontiDlg::valitseHakemisto);
    connect( ui->tuoNappi, &QPushButton::clicked, this, &VanhatuontiDlg::tuo);
}

VanhatuontiDlg::~VanhatuontiDlg()
{
    if( kpdb_.isOpen())
        kpdb_.close();

    delete ui;
}

void VanhatuontiDlg::alustaTuonti()
{
    QSettings settings("Kitupiikki Kirjanpito", "Kitupiikki");
    QVariantMap kirjanpidot = settings.value("Tietokannat").toMap();

    // Poistetaan ne, joita ei löydy
    for(QString polku : kirjanpidot.keys()) {
        if( QFile::exists(polku))
            kirjanpidot.remove("polku");
    }

    QMapIterator<QString, QVariant> iter(kirjanpidot);
    while( iter.hasNext()) {
        iter.next();
        QString polku = iter.key();
        QVariantList lista = iter.value().toList();
        QString nimi = lista.value(0).toString();
        QByteArray logo = lista.value(1).toByteArray();

        QListWidgetItem *item = new QListWidgetItem(nimi, ui->lista);
        item->setData(Qt::UserRole, polku);
        QPixmap kuva;
        kuva.loadFromData(logo, "PNG");
        item->setIcon(QIcon(kuva));
    }
}

void VanhatuontiDlg::tuoTiedostosta()
{
    QString tiedosto = QFileDialog::getOpenFileName(this, tr("Valitse tuotava kirjanpitotiedosto"),
                                                    QDir::homePath(),"Kirjanpito (*.kitupiikki kitupiikki.sqlite)");
    if( !tiedosto.isEmpty())
        avaaTietokanta(tiedosto);
}

void VanhatuontiDlg::haeTilikartta(const QString &polku)
{
    kitsasAsetukset_ = UusiVelho::asetukset(polku);

    // Tilit oma json-tiedosto
    {
        QFile tilit(polku + "/tilit.json");
        if( tilit.open(QIODevice::ReadOnly) ) {
            QJsonDocument doc = QJsonDocument::fromJson(tilit.readAll());
            QVariant variant = doc.toVariant();
            kitsasTilit_ = variant.toList();
        }
    }

    // TODO: Tilien muuntotaulukko (tarvitaan, koska yritystilikartassa tilinumerointia on muutettu)
}

void VanhatuontiDlg::avaaTietokanta(const QString &tiedostonnimi)
{
    ui->pino->setCurrentIndex(VIRHE);

    if( QSqlDatabase::connectionNames().contains("Tuonti"))
        kpdb_ = QSqlDatabase::database("Tuonti", false);
    else
        kpdb_ = QSqlDatabase::addDatabase("QSQLITE","Tuonti");

    kpdb_.setDatabaseName(tiedostonnimi);
    if( !kpdb_.open() ) {
        ui->virheLabel->setText(tr("Valitsemaasi tiedostoa %1 ei voi avata, tai se ei "
                                   "ole SQLITE-tietokanta.\n\n"
                                   "%2")
                                .arg(tiedostonnimi)
                                .arg(kpdb_.lastError().text()));
        return;
    }

    // Ladataan asetukset
    QSqlQuery query(kpdb_);
    query.exec("SELECT avain,arvo, muokattu FROM Asetus");
    if( !query.lastError().isValid()) {
        if( query.lastError().text().contains("locked")) {
            ui->virheLabel->setText(tr("Valitsemaasi tietokantaa ei voi avata, koska "
                                    "se on toisen ohjelman käytössä."));
        } else {
            ui->virheLabel->setText(tr("Tiedoston %1 avaamisessa tapahtui virhe tai valitsemasi "
                                       "tiedosto ei ole Kitupiikin tietokanta \n\n"
                                       "%2")
                                    .arg(tiedostonnimi)
                                    .arg(query.lastError().text()));
        }
    }
    while( query.next()) {
        kitupiikkiAsetukset_.insert(query.value(0).toString(), query.value(1).toString());
        if( !query.value(2).isNull())
            muokatutAsetukset_.append(query.value(0).toString());
    }

    if( !kitupiikkiAsetukset_.contains("KpVersio"))
        ui->virheLabel->setText(tr("Valitsemasi tiedosto ei ole Kitupiikin kirjanpito."));
    else if( kitupiikkiAsetukset_.value("KpVersio").toInt() >= 20)
        ui->virheLabel->setText(tr("Valitsemasi tiedosto on jo Kitsaan kirjanpito"));
    else if( kitupiikkiAsetukset_.value("KpVersio").toInt() < 10)
        ui->virheLabel->setText(tr("Valitsemaasi tiedosto on tallennettu vanhalla Kitupiikin versiolla.\n\n"
                                   "Tiedosto on ensin avattava uudemmalla Kitupiikin versiolla, jotta se "
                                   "päivittyy uudempaan tiedostomuotoon"));
    else if( kitupiikkiAsetukset_.value("VakioTilikartta") == "asoy.kpk" )
        ui->virheLabel->setText(tr("Asunto-osakeyhtiön tilikarttaa ei vielä ole saatavilla Kitsaaseen, "
                                   "eikä kirjanpito voi siksi tuoda."));
    else if( kitupiikkiAsetukset_.value("VakioTilikartta") != "tilitin.kpk"
             && kitupiikkiAsetukset_.value("VakioTilikartta") != "yhdistys-1.kpk")
        ui->virheLabel->setText(tr("Tuontitoiminto ei tue kirjanpitosi tilikarttaa tyyppiä %1 (%2).\n\n"
                                   "Tuontitoiminto tukee ainoastaa uusimpia elinkeinotoiminnan ja "
                                   "yhdistyksen tilikarttoja.\n\n"
                                   "Kirjanpidot, jotka on aloitettu Kitupiikin vanhimmilla versioilla, "
                                   "voi siirtää Kitsaaseen ainoastaan luomalla uuden kirjanpidon ja "
                                   "tekemällä siihen tilinavauksen vanhan kirjanpidon pohjalta.")
                                .arg(kitupiikkiAsetukset_.value("TilikarttaNimi"))
                                .arg(kitupiikkiAsetukset_.value("VakioTilikartta")));
    else
        alustaValinnat();
}

void VanhatuontiDlg::alustaValinnat()
{
    ui->pino->setCurrentIndex(VALINNAT);
    QString kitupiikkiTilikartta = kitupiikkiAsetukset_.value("VakioTilikartta");

    if( kitupiikkiTilikartta == "yhdistys-1.kpk")
        haeTilikartta(":/tilikartat/yhdistys");
    else if( kitupiikkiTilikartta == "tilitin.kpk")
        haeTilikartta(":/tilikartat/yritys");

    ui->nimiEdit->setText( kitupiikkiAsetukset_.value("Nimi"));

    // Laajuuksien hakeminen
    QVariantMap laajuusMap = kitsasAsetukset_.value("laajuudet").toMap();
    QMapIterator<QString,QVariant> laajuusIter(laajuusMap);
    while( laajuusIter.hasNext()) {
        laajuusIter.next();
        KieliKentta kk( laajuusIter.value() );
        QListWidgetItem *item = new QListWidgetItem(kk.teksti(), ui->laajuusLista);
        item->setData(Qt::UserRole, laajuusIter.key());
    }


    if( kitupiikkiTilikartta == "yhdistys-1.kpk") {
        ui->tilikarttaLabel->setText(tr("Yhdistys"));
        int laajuus = kitupiikkiAsetukset_.value("Muoto").left(1).toInt();
        if( laajuus > 3)
            laajuus++;  // Väliin tullut yksi laajuus
        kitsasAsetukset_.insert("laajuus", laajuus);
        kitsasAsetukset_.insert("muoto","ry");
    } else if( kitupiikkiTilikartta == "tilitin.kpk") {
        // Tieto yrityksen muodosta siirtyy muodoksi
        QString vanhamuoto = kitupiikkiAsetukset_.value("Muoto");
        QString uusimuoto = "oy";

        if( vanhamuoto == "Avoin yhtiö")
            uusimuoto = "ay";
        else if( vanhamuoto == "Elinkeinonharjoittaja")
            uusimuoto = "tmi";
        else if( vanhamuoto == "Kommandiittiyhtiö")
            uusimuoto = "ky";
        else if( vanhamuoto == "Osuuskunta")
            uusimuoto = "osk";
        // Ei enää eroa julkisen ja muun oy:n välillä
        else if( vanhamuoto == "Julkinen osakeyhtiö")
            vanhamuoto = "Osakeyhtiö";
        ui->tilikarttaLabel->setText(tr("Yritys (%1)").arg(vanhamuoto));
        kitsasAsetukset_.insert("muoto", uusimuoto);
    }

    ui->laajuusLista->setCurrentRow( kitsasAsetukset_.value("laajuus").toInt() - 1);

    bool muokattuja = !muokatutAsetukset_.filter("Raportti/").isEmpty() ||
                      muokatutAsetukset_.contains("TilinpaatosPohja");
    ui->muovaKuva->setVisible(muokattuja);
    ui->muvaLabel->setVisible(muokattuja);
    ui->jatkaNappi->setVisible(true);
}

void VanhatuontiDlg::alustaSijainti()
{
    ui->pino->setCurrentIndex(SIJAINTI);

    ui->jatkaNappi->setVisible(false);
    ui->tuoNappi->setVisible(true);
    QString vanhasijainti = kpdb_.databaseName();

    QFileInfo info(vanhasijainti);
    QString uusinimi = info.fileName();
    uusinimi.remove(".kitupiikki");

    ui->tiedostonNimi->setValidator(new QRegularExpressionValidator(QRegularExpression("([A-Za-z0-9]|-){1,64}")));

    ui->tiedostonNimi->setText( uusinimi );
    ui->tiedostonHakemisto->setText( info.dir().absolutePath() );
}

void VanhatuontiDlg::valitseHakemisto()
{
    QString hakemisto = QFileDialog::getExistingDirectory(this, tr("Valitse tallennushakemisto"),
                                                          ui->tiedostonHakemisto->text());
    if( !hakemisto.isEmpty())
        ui->tiedostonHakemisto->setText( hakemisto );
}

void VanhatuontiDlg::tuo()
{
    QDir hakemisto(ui->tiedostonHakemisto->text());
    QString polku = hakemisto.absoluteFilePath(ui->tiedostonNimi->text() + ".kitsas");
    if( ui->tiedostonNimi->text().isEmpty() || polku.isEmpty()) {
        QMessageBox::critical(this, tr("Kirjanpidon tuonti"), tr("Tiedoston nimi ei kelpaa"));
        return;
    } else if( QFile::exists(polku)) {
        QMessageBox::critical(this, tr("Kirjanpidon tuonti"), tr("Tiedosto %1 on jo olemassa.").arg(polku));
        return;
    }

    ui->tuoNappi->setEnabled(false);
    ui->peruNappi->setEnabled(false);
    ui->pino->setCurrentIndex(ODOTA);
    qApp->processEvents();

    // Määritellään etenemismittaria
    int laskenta = 100;
    QSqlQuery sql( kpdb_);
    sql.exec("SELECT COUNT(id) FROM Tosite");
    if( sql.next())
        laskenta += sql.value(0).toInt();
    sql.exec("SELECT COUNT(id) FROM Liite");
    if( sql.next())
        laskenta += sql.value(0).toInt();
    ui->progressBar->setRange(0, laskenta);

    // Tietokannan luoneen version tieto
    ui->progressBar->setValue(2);
    kitsasAsetukset_.insert("KpVersio", Kirjanpito::TIETOKANTAVERSIO);
    kitsasAsetukset_.insert("LuotuVersiolla", qApp->applicationVersion());

    QVariantMap initMap;
    initMap.insert("asetukset", kitsasAsetukset_);
    initMap.insert("tilit", kitsasTilit_);
    QVariantMap map;
    map.insert("name", ui->nimiEdit->text());
    map.insert("init", initMap);

    if( !kp()->sqlite()->uusiKirjanpito(polku, map)) {
        ui->pino->setCurrentIndex(VIRHE);
        ui->virheLabel->setText(tr("Uuden tietokannan luominen epäonnistui."));
        return;
    }
    ui->progressBar->setValue(10);

    // Muuten avataan tietokanta
    if( !kp()->sqlite()->avaaTiedosto(polku)) {
        ui->pino->setCurrentIndex(VIRHE);
        ui->virheLabel->setText(tr("Uuden tietokannan luominen epäonnistui."));
        return;
    }
    qApp->processEvents();

    ui->progressBar->setValue(40);
    siirraAsetukset();
    ui->progressBar->setValue(50);
    siirraTilikaudet();
    ui->progressBar->setValue(55);
    taydennaTilit();
    ui->progressBar->setValue(70);
    siirraTuotteet();
    ui->progressBar->setValue(80);
    siirraAsiakkaat();
    ui->progressBar->setValue(90);
    siirraTositteet();
    siirraLiiteet();

    // Tietokanta pitää avata vielä uudelleen, jotta päivittää tiedot
    // tietokannasta
    kp()->sqlite()->sulje();
    kp()->sqlite()->avaaTiedosto(polku);

    ui->pino->setCurrentIndex(VALMIS);
    ui->peruNappi->setEnabled(true);
}

void VanhatuontiDlg::siirraAsetukset()
{
    QVariantMap map;


    QStringList siirrettavat;
    siirrettavat << "AlvVelvollinen" << "Harjoitus" << "Kotipaikka" << "LaskuSeuraavaId" << "LogossaNimi"
                 << "Nimi" << "Osoite" << "Puhelin" << "Tilinavaus" << "TilinavausPvm" << "TilitPaatetty" << "Ytunnus";

    // Joitain laskutuksen valintoja voisi myös siirtää


    for(auto siirrettava : siirrettavat)
        if( kitupiikkiAsetukset_.contains(siirrettava))
            map.insert(siirrettava, kitupiikkiAsetukset_.value(siirrettava));


    map.insert("AlvAlkaa", QDate::fromString(kitupiikkiAsetukset_.value("TilitPaatetty"), Qt::ISODate).addDays(1));
    map.insert("Email", kitupiikkiAsetukset_.value("Sahkoposti"));


    KpKysely *kysely = kpk("/asetukset", KpKysely::PATCH);
    kysely->kysy(map);

}

void VanhatuontiDlg::siirraTilikaudet()
{
    QSqlQuery sql(kpdb_);
    sql.exec("SELECT alkaa, loppuu, json FROM Tilikausi ORDER BY alkaa");
    while( sql.next()) {
        KpKysely *kysely = kpk(QString("/tilikaudet/%1").arg(sql.value(0).toDate().toString(Qt::ISODate)), KpKysely::PUT);
        QVariantMap map;
        map.insert("loppuu", sql.value(1));

        // TODO Muut ominaisuudet

        kysely->kysy(map);
        // TODO Budjetti
    }
}

void VanhatuontiDlg::taydennaTilit()
{
    QSqlQuery sql(kpdb_);
    sql.exec("SELECT nro, nimi, tyyppi, tila, json, muokattu, id FROM Tili");
    while( sql.next()) {
        int id = sql.value("id").toInt();
        int numero = tilimuunto(sql.value(0).toInt());

        tiliIdlla_.insert(id, numero);

        QString tyyppi = sql.value(2).toString();

        bool loytyi = false;
        if( sql.value("muokattu").isNull()) {
            for( auto item : kitsasTilit_) {
                QVariantMap tmap = item.toMap();
                if( (!tyyppi.startsWith('H') || tyyppi == tmap.value("tyyppi").toString() )
                     && numero == tmap.value("numero").toInt() ) {
                    loytyi = true;
                    break;
                }
            }
        }
        // Jos Kitsaassa ei ole vastaavaa tiliä se lisätään.
        // Samoin muokkaukseen päädytään, jos tiliä on muokattu
        if( !loytyi ) {
            QVariantMap tmap;
            tmap.insert("numero", numero);
            tmap.insert("tyyppi", tyyppi);

            QVariantMap nimiMap;
            nimiMap.insert("fi", sql.value("nimi"));
            tmap.insert("nimi", nimiMap);

            // Nyt JSONista pitäisi hakea kaikenlaista mielenkiintoista ;)
            QVariantMap jsonmap = QJsonDocument::fromJson( sql.value("json").toByteArray() ).toVariant().toMap();
            if( jsonmap.contains("AlvLaji"))
                tmap.insert("alvlaji", jsonmap.value("AlvLaji"));
            if( jsonmap.contains("AlvProsentti"))
                tmap.insert("alvprosentti", jsonmap.value("AlvProsentti"));
            if( jsonmap.contains("Kirjausohje"))
                tmap.insert("ohje", jsonmap.value("Kirjausohje"));
            if( jsonmap.contains("Tasaerapoisto"))
                tmap.insert("tasaerapoisto", jsonmap.value("Tasaerapoisto"));
            if( jsonmap.contains("Menojaannospoisto"))
                tmap.insert("menojaannospoisto", jsonmap.value("Menojaannospoisto"));
            if( jsonmap.contains("Poistotili"))
                tmap.insert("poistotili", tilimuunto(jsonmap.value("Poistotili").toInt()));
            if( jsonmap.contains("Taseerittely"))
                tmap.insert("erittely", jsonmap.value("Taseerittely"));
            if( jsonmap.contains("IBAN"))
                tmap.insert("IBAN", jsonmap.value("IBAN"));

            KpKysely *kysely = kpk("/tilit", KpKysely::PUT);
            kysely->kysy(tmap);
        }
    }
}

void VanhatuontiDlg::siirraKohdennukset()
{
    QSqlQuery sql(kpdb_);
    sql.exec("SELECT id, nimi, tyyppi, alkaa, loppuu, tyyppi, json");
    while( sql.next()) {
        int id = sql.value(0).toInt();
        int tyyppi = sql.value("tyyppi").toInt();
        if( id == 0)
            continue;
        QVariantMap kmap;
        QVariantMap nimimap;
        nimimap.insert("fi", sql.value(1).toString());
        kmap.insert("nimi",nimimap);
        kmap.insert("tyyppi", tyyppi);
        kmap.insert("alkaa", sql.value("alkaa"));
        kmap.insert("loppuu", sql.value("loppuu"));
        if( tyyppi == 2 )   // Projekti
            kmap.insert("kuuluu", QVariant());

        KpKysely* kysely = kpk( QString("/kohdennukset/%1").arg(id), KpKysely::PUT);
        kysely->kysy(kmap);
    }
}


void VanhatuontiDlg::siirraTuotteet()
{
    QSqlQuery sql(kpdb_);
    sql.exec("SELECT id, nimike, yksikko, hintaSnt, alvkoodi, alvprosentti, kohdennus FROM Tuote");
    while( sql.next()) {
        QVariantMap tmap;
        tmap.insert("nimike", sql.value("nimike"));
        tmap.insert("yksikko", sql.value("yksikko"));
        tmap.insert("ahinta", sql.value("hintaSnt").toDouble() * 100.0);
        tmap.insert("alvkoodi", sql.value("alvkoodi").toInt());
        tmap.insert("alvprosentti", sql.value("alvprosentti").toDouble());
        tmap.insert("kohdennus", sql.value("kohdennus").toInt());

        KpKysely *kysely = kpk(QString("/tuotteet/%1").arg(sql.value("id").toInt()), KpKysely::PUT);
        kysely->kysy(tmap);
    }
}

void VanhatuontiDlg::siirraAsiakkaat()
{
    // TODO THIS !!!!
}

void VanhatuontiDlg::siirraTositteet()
{
    QSqlQuery tositekysely(kpdb_);
    tositekysely.exec("SELECT Tosite.id as id, pvm, otsikko, kommentti, tunniste, tosite.json as json, tunnus FROM Tosite JOIN Tositelaji ON Tosite.laji=Tositelaji.id");
    QSqlQuery vientikysely(kpdb_);
    QSqlQuery merkkauskysely( kpdb_ );
    while( tositekysely.next()) {
        int tositeid = tositekysely.value("id").toInt();
        Tosite tosite;
        tosite.setData(Tosite::ID, tositeid);
        tosite.asetaPvm(tositekysely.value("pvm").toDate());
        tosite.asetaTyyppi(TositeTyyppi::TUONTI);
        tosite.asetaOtsikko(tositekysely.value("otsikko").toString());
        tosite.setData(Tosite::KOMMENTIT, tositekysely.value("kommentti"));
        tosite.setData(Tosite::SARJA, tositekysely.value("tunnus"));
        tosite.setData(Tosite::TUNNISTE, tositekysely.value("tunniste"));

        vientikysely.exec(QString("SELECT id, pvm, tili, debetsnt, kreditsnt, selite, alvkoodi, alvprosentti, "
                                  "kohdennus, eraid, viite, iban, laskupvm, erapvm, "
                          "arkistotunnus, asiakas, json FROM Vienti WHERE tosite=%1 ORDER BY vientirivi").arg(tositeid));
        while(vientikysely.next()) {
            TositeVienti vienti;
            int vientiid = vientikysely.value("id").toInt();
            vienti.setId( vientiid );
            vienti.setPvm( vientikysely.value("pvm").toDate());
            vienti.setTili( tiliIdlla_.value( vientikysely.value("tili").toInt() ));
            qlonglong debetsnt = vientikysely.value("debetsnt").toLongLong();
            qlonglong kreditsnt = vientikysely.value("kreditsnt").toLongLong();
            if( debetsnt )
                vienti.setDebet(debetsnt);
            else if( kreditsnt)
                vienti.setKredit( kreditsnt );
            vienti.setSelite( vientikysely.value("selite").toString());
            vienti.setAlvKoodi( vientikysely.value("alvkoodi").toInt());
            vienti.setAlvProsentti( vientikysely.value("alvprosentti").toDouble());
            vienti.setKohdennus( vientikysely.value("kohdennus").toInt());
            vienti.setEra( vientikysely.value("eraid").toInt());
            vienti.setViite( vientikysely.value("viite").toString());
            vienti.setErapaiva( vientikysely.value("erapvm").toDate());
            vienti.setArkistotunnus( vientikysely.value("arkistotunnus").toString());
            // TODO: Asiakas ja laskut yms json-jutut

            // TODO: Merkkaukset
            QVariantList merkkaukset;
            merkkauskysely.exec(QString("SELECT kohdennus FROM Merkkaus WHERE vienti=%1").arg(vientiid));
            while( merkkauskysely.next() ) {
                merkkaukset.append( merkkauskysely.value(0) );
            }
            if( !merkkaukset.isEmpty())
                vienti.setMerkkaukset(merkkaukset);

            tosite.viennit()->lisaa(vienti);
        }
        connect(&tosite, &Tosite::tallennusvirhe, [] (int virhe) { qDebug() << "TOSITEVIRHE " << virhe;});
        tosite.tallenna();
        ui->progressBar->setValue( ui->progressBar->value() + 1 );
        qApp->processEvents();
    }
}

void VanhatuontiDlg::siirraLiiteet()
{
    QSqlQuery sql(kpdb_);
    sql.exec("SELECT tosite, otsikko, data FROM Liite ORDER BY tosite, liiteno");
    while( sql.next()) {
        int tosite = sql.value(0).toInt();
        QString otsikko = sql.value(1).toString();
        QByteArray data = sql.value(2).toByteArray();

        if( tosite == 0) {
            KpKysely* kysely = kpk(QString("/liitteet/0/%1").arg(otsikko), KpKysely::PUT);
            kysely->lahetaTiedosto(data);
        } else {
            KpKysely* kysely = kpk(QString("/liitteet/%1").arg(tosite), KpKysely::POST);
            kysely->lahetaTiedosto(data);
        }
        ui->progressBar->setValue( ui->progressBar->value() + 1 );
        qApp->processEvents();
    }
}


int VanhatuontiDlg::tilimuunto(int tilinumero) const
{
    return tilinMuunto_.value(tilinumero, tilinumero);
}

