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
#include "pilveensiirto.h"
#include "ui_pilveensiirto.h"

#include "db/kirjanpito.h"
#include "pilvimodel.h"
#include "pilvikysely.h"
#include "sqlite/sqlitemodel.h"
#include "tilaus/planmodel.h"

#include <QSqlQuery>
#include <QImage>
#include <QPixmap>
#include <QPushButton>
#include <QMessageBox>
#include <QDebug>
#include <QSettings>
#include <QTimer>

PilveenSiirto::PilveenSiirto(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::PilveenSiirto),
    pilviModel_(new PilviModel(this, kp()->pilvi()->token()))
{

    setAttribute(Qt::WA_DeleteOnClose);

    ui->setupUi(this);
    alustaAlkusivu();

    connect( ui->buttonBox->button(QDialogButtonBox::Cancel), &QPushButton::clicked, this, &PilveenSiirto::close);
    connect( ui->buttonBox->button(QDialogButtonBox::Ok), &QPushButton::clicked, this, &PilveenSiirto::accept);
    connect( ui->buttonBox, &QDialogButtonBox::helpRequested, [] { kp()->ohje("aloittaminen/pilvi"); });
    connect( ui->toimistoCombo, &QComboBox::currentIndexChanged, this, &PilveenSiirto::toimistoValittu);
    connect( ui->toimistoRadio, &QRadioButton::toggled, this, &PilveenSiirto::paivitaInfot);

    ui->sijaintiGroup->hide();

    haeToimistot();
}

PilveenSiirto::~PilveenSiirto()
{
    delete ui;
}

void PilveenSiirto::accept()
{
    if( ui->stackedWidget->currentIndex() == VALMIS) {
        close();
    } else {
        ui->rasti1->hide();
        ui->rasti2->hide();
        ui->stackedWidget->setCurrentIndex(KAYNNISSA);
        ui->buttonBox->hide();
        ui->progressBar->setRange(0, tositelkm_ + liitelkm_ + 50);
        ui->progressBar->setValue(1);

        KpKysely *init = kpk("/init");
        connect( init, &KpKysely::vastaus, this, &PilveenSiirto::initSaapuu);        
        init->kysy();
    }
}

void PilveenSiirto::alustaAlkusivu()
{    

    ui->nimiLabel->setText( kp()->asetukset()->nimi() );
    if( !kp()->logo().isNull())
        ui->logoLabel->setPixmap( QPixmap::fromImage( kp()->logo().scaled(32,32,Qt::KeepAspectRatio)) );


    QSqlQuery kysely(kp()->sqlite()->tietokanta());
    kysely.exec("SELECT COUNT(id) FROM Tosite");
    kysely.next();
    tositelkm_ = kysely.value(0).toInt();

    kysely.exec("SELECT COUNT(id) FROM Liite");
    kysely.next();
    liitelkm_ = kysely.value(0).toInt();

    paivitaInfot();

    kysely.exec("SELECT pvm, sarja, tunniste, nimi, LENGTH(data) AS koko FROM Liite LEFT OUTER JOIN Tosite ON Liite.tosite=Tosite.id WHERE LENGTH(data) > 10 * 1024 * 1024");
    while( kysely.next())
    {
        qlonglong koko = kysely.value("koko").toLongLong();
        qlonglong mt = koko / ( 1024 * 1024 );
        ui->isoLista->addItem(QString("%1 %2 (%3 Mt)")
                              .arg( kp()->tositeTunnus(kysely.value("tunniste").toInt(), kysely.value("pvm").toDate(), kysely.value("sarja").toString()) )
                              .arg( kysely.value("nimi").toString())
                              .arg( mt ) );
    }
    if( ui->isoLista->count()) {
        ui->stackedWidget->setCurrentIndex(YLIISO);
        ui->buttonBox->button(QDialogButtonBox::Ok)->hide();
    }
}



void PilveenSiirto::initSaapuu(QVariant *data)
{
    QVariantMap init = data->toMap();

    // Poistetaan Yleinen-kohdennus, joka on valmiina tietokannassa
    QVariantList kohdennukset = init.value("kohdennukset").toList();
    kohdennukset.removeFirst();
    init.insert("kohdennukset", kohdennukset);

    QVariantMap asetukset = init.value("asetukset").toMap();
    if( kp()->settings()->value("SmtpServer").toString().isEmpty() &&
        asetukset.value("SmtpServer").toString().isEmpty() ) {
        // Otetaan käyttöön pilven email-palvelu
        asetukset.insert("KitsasEmail", true);
        asetukset.insert("EmailOsoite", asetukset.contains("Email") ? asetukset.value("Email") :  kp()->pilvi()->kayttaja().email() );
        asetukset.insert("EmailNimi", asetukset.value("Nimi"));
        init.insert("asetukset", asetukset);
    }

    QVariantMap map;
    map.insert("name", kp()->asetukset()->nimi());
    map.insert("businessid", kp()->asetukset()->ytunnus());
    map.insert("trial", kp()->onkoHarjoitus());
    map.insert("init", init);

    if( ui->toimistoRadio->isChecked()){
        const QString& location = ui->hyllyCombo->currentData().toString();
        if( !location.isEmpty() ) {
            map.insert("location", location);
        }
    }

    PilviKysely* kysely = new PilviKysely(pilviModel_, KpKysely::POST, pilviModel_->pilviLoginOsoite() + "/clouds");
    connect( kysely, &PilviKysely::vastaus, this, &PilveenSiirto::pilviLuotu);
    connect( kysely, &KpKysely::virhe, this, &PilveenSiirto::siirtoVirhe);
    kysely->kysy(map);
    ui->progressBar->setValue(10);

    for(const auto& tk : init.value("tilikaudet").toList()) {
        QVariantMap tkmap = tk.toMap();
        tilikaudet.append( tkmap.value("alkaa").toString());
    }
}

void PilveenSiirto::pilviLuotu(QVariant *data)
{
    const QVariantMap& map = data->toMap();
    pilviId_ = map.value("id").toInt();

    QVariant info = map.value("info");

    connect( pilviModel_, &PilviModel::siirtoPilviAvattu, this, &PilveenSiirto::avaaLuotuPilvi  );
    pilviModel_->alustaPilvi( &info, true);

    /*
    connect( pilviModel_, &PilviModel::siirtoPilviAvattu, this, &PilveenSiirto::avaaLuotuPilvi  );
    ui->progressBar->setValue(20);

    pilviModel_->avaaPilvesta(pilviId_, true);
    */

}

void PilveenSiirto::avaaLuotuPilvi()
{   
    ui->progressBar->setValue(30);
    haeRyhmaLista();
}

void PilveenSiirto::haeRyhmaLista()
{
    KpKysely *kysely = kpk("/ryhmat");
    if( kysely ) {
        connect(kysely, &KpKysely::vastaus, this, &PilveenSiirto::ryhmaListaSaapuu);
        kysely->kysy();
    }
}

void PilveenSiirto::ryhmaListaSaapuu(QVariant *data)
{
    ryhmat = data->toList();
    ui->progressBar->setValue(40);
    tallennaSeuraavaRyhma();
}

void PilveenSiirto::tallennaSeuraavaRyhma()
{
    if( ryhmat.isEmpty()) {
        haeKumppaniLista();
        return;
    }
    QVariantMap map = ryhmat.takeFirst().toMap();
    int id = map.take("id").toInt();

    PilviKysely *tallennus = new PilviKysely(pilviModel_, KpKysely::PUT, QString("/ryhmat/%1").arg(id));
    connect( tallennus, &KpKysely::vastaus, this, &PilveenSiirto::tallennaSeuraavaRyhma);
    connect( tallennus, &KpKysely::virhe, this, &PilveenSiirto::siirtoVirhe);
    tallennus->kysy(map);
}

void PilveenSiirto::haeKumppaniLista()
{
    KpKysely *kaverikysely = kpk("/kumppanit");
    connect( kaverikysely, &KpKysely::vastaus, this, &PilveenSiirto::kumppaniListaSaapuu);
    kaverikysely->kysy();
}

void PilveenSiirto::kumppaniListaSaapuu(QVariant *data)
{
    const QVariantList& lista = data->toList();
    for( const auto& item : qAsConst( lista )) {
        QVariantMap map = item.toMap();
        int id = map.value("id").toInt();
        QString nimi = map.value("nimi").toString();
        if(id && !nimi.isEmpty())
            kumppanit.enqueue(id);
    }
    kysySeuraavaKumppani();
}

void PilveenSiirto::kysySeuraavaKumppani()
{
    if( kumppanit.isEmpty()) {
        ui->progressBar->setValue(50);
        haeTositeLista();
        return;
    }
    int id = kumppanit.dequeue();
    KpKysely *kaverikysely = kpk(QString("/kumppanit/%1").arg(id));
    connect(kaverikysely, &KpKysely::vastaus, this, &PilveenSiirto::tallennaKumppani);
    kaverikysely->kysy();
}

void PilveenSiirto::tallennaKumppani(QVariant *data)
{
    QVariantMap map = data->toMap();
    int id = map.take("id").toInt();

    if( map.value("nimi").toString() == "Verohallinto") {
        kysySeuraavaKumppani();
        return;
    }

    PilviKysely *tallennus = new PilviKysely(pilviModel_, KpKysely::PUT, QString("/kumppanit/%1").arg(id));
    connect(tallennus, &KpKysely::vastaus, this, &PilveenSiirto::kysySeuraavaKumppani);
    connect(tallennus, &KpKysely::virhe, this, &PilveenSiirto::siirtoVirhe);
    tallennus->kysy(map);
}

void PilveenSiirto::haeTositeLista()
{
    ui->rasti1->show();
    ui->vaihe2->setEnabled(true);

    QSqlQuery tositeKysely(kp()->sqlite()->tietokanta());
    tositeKysely.exec("SELECT id FROM tosite ORDER BY id");
    while( tositeKysely.next())
        tositteet.enqueue(tositeKysely.value(0).toInt());
    kysySeuraavaTosite();
}

void PilveenSiirto::kysySeuraavaTosite()
{
    if( tositteet.isEmpty()) {
        tallennaLiitteet();
        return;
    }
    int id = tositteet.dequeue();
    KpKysely *kysely = kpk(QString("/tositteet/%1").arg(id));
    connect(kysely, &KpKysely::vastaus, this, &PilveenSiirto::tallennaTosite);
    kysely->kysy();
}

void PilveenSiirto::tallennaTosite(QVariant *data)
{

    QVariantMap map = data->toMap();
    int id = map.take("id").toInt();

    map.remove("loki");
    map.remove("liitteet");

    if( map.contains("lasku")) {
        // lasku.numero on vanhoissa kirjanpidoissa tyypiltään string
        // ja niin se yhä edelleen saa olla ;)
        QVariantMap laskuMap = map.value("lasku").toMap();
        if( laskuMap.value("viivkorko").toDouble() > 1e-5) {
            laskuMap.insert("viivkorko", laskuMap.value("viivkorko").toDouble());
        }
        QStringList keys = laskuMap.keys();
        for( const auto& key : qAsConst( keys )) {
            if( laskuMap.value(key).isNull())
                laskuMap.remove(key);
        }
        map.insert("lasku", laskuMap);
    }

    PilviKysely *tallennus = new PilviKysely(pilviModel_, KpKysely::PUT, QString("/tositteet/%1").arg(id));
    connect(tallennus, &KpKysely::vastaus, this, &PilveenSiirto::kysySeuraavaTosite);
    connect(tallennus, &KpKysely::virhe, this, &PilveenSiirto::siirtoVirhe);
    tallennus->kysy(map);
    ui->progressBar->setValue( ui->progressBar->value() + 1);
}

void PilveenSiirto::tallennaLiitteet()
{
    ui->rasti2->show();
    ui->vaihe3->setEnabled(true);

    liitekysely = QSqlQuery( kp()->sqlite()->tietokanta() );
    liitekysely.exec("SELECT id, nimi, tyyppi, data, tosite, roolinimi FROM Liite "
                     "WHERE tosite IS NOT NULL OR roolinimi IS NOT NULL");
    tallennaSeuraavaLiite();

}

void PilveenSiirto::tallennaSeuraavaLiite()
{
    qApp->processEvents();

    if( liitekysely.next()) {
        yrityksia_ = 3;
        tallennaTamaLiite();
    } else {
        tallennaBudjetit();
        tallennaVakioviitteet();
    }
}

void PilveenSiirto::tallennaTamaLiite()
{
    QString nimi = liitekysely.value("nimi").toString();
    QString tyyppi = liitekysely.value("tyyppi").toString();
    QString rooli = liitekysely.value("roolinimi").toString();
    QByteArray data = liitekysely.value("data").toByteArray();
    int tosite = liitekysely.value("tosite").toInt();

    if(tyyppi.isEmpty())
        tyyppi = KpKysely::tiedostotyyppi(data);

    QMap<QString,QString> meta;
    meta.insert("Content-type", tyyppi);
    meta.insert("Filename", nimi);

    KpKysely *kysely = rooli.isEmpty() ?
                           new PilviKysely(pilviModel_, KpKysely::POST, QString("/liitteet/%1").arg(tosite)) :
                           new PilviKysely(pilviModel_, KpKysely::PUT, QString("/liitteet/%1/%2").arg(tosite).arg(rooli));
    connect(kysely, &KpKysely::vastaus, this, &PilveenSiirto::tallennaSeuraavaLiite);
    connect(kysely, &KpKysely::virhe, this, &PilveenSiirto::liiteEpaonnistui);
    kysely->lahetaTiedosto(data, meta);
    ui->progressBar->setValue(ui->progressBar->value()+1);

}

void PilveenSiirto::liiteEpaonnistui(int virhe)
{
    yrityksia_--;
    if(yrityksia_ < 0) {
        siirtoVirhe(virhe);
    } else {
        qCritical() << "Siirtovirhe " << virhe;
        QTimer::singleShot(10000, this, &PilveenSiirto::tallennaTamaLiite);
    }
}

void PilveenSiirto::tallennaBudjetit()
{
    for(const QString& kausi : qAsConst(tilikaudet)) {
        KpKysely *haku = kpk(QString("/budjetti/%1").arg(kausi));
        haku->lisaaAttribuutti("kohdennukset");
        connect( haku, &KpKysely::vastaus, this, [this, kausi] (QVariant *data) { this->tallennaBudjetti(kausi, data); } );
        haku->kysy();
    }
}

void PilveenSiirto::tallennaBudjetti(const QString& tilikausi, QVariant* data) {
    QVariantMap map = data->toMap();
    if( !map.isEmpty() ) {
        KpKysely *tallennus = new PilviKysely(pilviModel_, KpKysely::PUT, QString("/budjetti/%1").arg(tilikausi));
        tallennus->kysy(map);
    }
}

void PilveenSiirto::tallennaVakioviitteet()
{
    KpKysely *haku = kpk("/vakioviitteet");
    connect(haku, &KpKysely::vastaus, this, &PilveenSiirto::vakioViitteetSaapuu);
    haku->kysy();
}

void PilveenSiirto::vakioViitteetSaapuu(const QVariant *data)
{
    vakioviitteet = data->toList();
    tallennaSeuraavaVakioViite();
}

void PilveenSiirto::tallennaSeuraavaVakioViite()
{
    if( vakioviitteet.isEmpty())
        tallennaTuotteet();
    else {
        QVariantMap map = vakioviitteet.takeFirst().toMap();
        int viite = map.value("viite").toInt();
        KpKysely *tallennus = new PilviKysely(pilviModel_, KpKysely::PUT, QString("/vakioviitteet/%1").arg(viite));
        connect(tallennus, &KpKysely::vastaus, this, &PilveenSiirto::tallennaSeuraavaVakioViite);
        connect(tallennus, &KpKysely::virhe, this, &PilveenSiirto::siirtoVirhe);
        tallennus->kysy(map);
    }
}

void PilveenSiirto::tallennaTuotteet()
{
    KpKysely *haku = kpk("/tuotteet");
    connect(haku, &KpKysely::vastaus, this, &PilveenSiirto::tuotteetSaapuu);
    haku->kysy();
}

void PilveenSiirto::tuotteetSaapuu(const QVariant *data)
{
    tuotteet_ = data->toList();
    tallennaSeuraavaTuote();
}

void PilveenSiirto::tallennaSeuraavaTuote()
{
    if( tuotteet_.isEmpty()) {
        // Varmistetaan vielä laskunumeroinnin oikea alkaminen
        QVariantMap map;
        qlonglong seuraavaId = kp()->asetukset()->isoluku("LaskuSeuraavaId");
        qlonglong numerointialkaa = kp()->asetukset()->isoluku("LaskuNumerointialkaa");
        map.insert("LaskuNumerointialkaa", seuraavaId > numerointialkaa ? seuraavaId : numerointialkaa);
        KpKysely *lst = new PilviKysely(pilviModel_, KpKysely::PATCH, "/asetukset");
        connect( lst, &KpKysely::vastaus, this, &PilveenSiirto::valmis);
        connect(lst, &KpKysely::virhe, this, &PilveenSiirto::siirtoVirhe);
        lst->kysy(map);
    } else {
        QVariantMap map = tuotteet_.takeFirst().toMap();
        int id = map.take("id").toInt();
        if( map.value("tili").toInt()) {
        KpKysely *tallennus = new PilviKysely(pilviModel_, KpKysely::PUT, QString("/tuotteet/%1").arg(id));
        connect(tallennus, &KpKysely::vastaus, this, &PilveenSiirto::tallennaSeuraavaTuote);
        connect(tallennus, &KpKysely::virhe, this, &PilveenSiirto::siirtoVirhe);
        tallennus->kysy(map);
        } else {
            tallennaSeuraavaTuote();
        }
    }
}

void PilveenSiirto::valmis(QVariant* /*data*/)
{ 

    PilviKysely* kysely = new PilviKysely(pilviModel_, KpKysely::GET, "/info");
    connect( kysely, &KpKysely::vastaus, this, &PilveenSiirto::infoSaapuu);
    kysely->kysy();
}

void PilveenSiirto::infoSaapuu(QVariant *data)
{
    QVariantMap map = data->toMap();
    int tositteita = map.value("tositteita").toInt();
    qlonglong koko = map.value("koko").toLongLong();
    const QString& uid = kp()->asetukset()->uid();

    if( tositteita != tositelkm_) {
        qDebug() << QString("Tositteita siirretty %1 / %2").arg(tositteita).arg(tositelkm_);
        siirtoVirhe(0);
    } else {
        pilviModel_->sulje();
        ui->buttonBox->show();
        ui->buttonBox->button(QDialogButtonBox::Cancel)->hide();
        ui->valmisInfo->setText(tr("Tositteita kopioitu %1 kpl, kirjanpidon koko pilvessä %L2 Mt")
                                .arg(tositteita)
                                .arg(koko / 1000000.0 ,0,'f',1 ));
        ui->stackedWidget->setCurrentIndex(VALMIS);

        kp()->pilvi()->paivitaLista(pilviId_);
//        kp()->pilvi()->avaaPilvesta(pilviId_);

        kp()->settings()->setValue("PilveenSiirretyt",
                                   kp()->settings()->value("PilveenSiirretyt").toString() + uid + " ");
    }
}

void PilveenSiirto::siirtoVirhe(int koodi)
{
    qDebug() << "Siirtovirhe " << koodi;
    ui->stackedWidget->setCurrentIndex(VALMIS);
    ui->buttonBox->show();
    ui->buttonBox->button(QDialogButtonBox::Cancel)->hide();
    if (koodi == 0)
        ui->valmisLabel->setText(tr("Kirjanpidon siirto pilveen epäonnistui.\n\n"
                                    "Kaikkia tositteita ei saatu siirrettyä pilveen."));
    else if( koodi == 302)
        ui->valmisLabel->setText(tr("Kirjanpidon siirto pilveen epäonnistui.\n\n"
                                    "Kirjanpitoa on mahdollisesti käsitelty sellaisella ohjelman versiolla, "
                                    "jonka jäljiltä tallenteessa on vähäinen tekninen virhe, joka on "
                                    "havaittu pilvipalvelun tarkemmissa tarkastuksissa.\n\n"
                                    "Tämän kirjanpidon kopioiminen pilveen vaatii kirjanpidon korjaamista "
                                    "ohjelmiston tuen tai muun asiantuntijan avulla."));
    else
        ui->valmisLabel->setText(tr("Kirjanpidon siirto pilveen epäonnistui virheen %1 takia").arg(koodi));
            pilviModel_->poistaNykyinenPilvi();
}

void PilveenSiirto::haeToimistot()
{
    PilviKysely* kysely = new PilviKysely(pilviModel_, KpKysely::GET, pilviModel_->pilviLoginOsoite() + "/v1/offices");
    connect( kysely, &PilviKysely::vastaus, this, &PilveenSiirto::toimistotSaapuu);
    kysely->kysy();
}

void PilveenSiirto::toimistotSaapuu(const QVariant *data)
{
    QVariantList lista = data->toList();
    ui->toimistoCombo->clear();
    for(const auto& item : lista) {
        const QVariantMap map = item.toMap();
        const auto& id = map.value("id").toString();
        const auto& name = map.value("name").toString();
        ui->toimistoCombo->addItem(name, id);
    }

    ui->sijaintiGroup->setVisible( !lista.isEmpty() );
    ui->toimistoCombo->model()->sort(0);

}

void PilveenSiirto::toimistoValittu()
{
    const QString id = ui->toimistoCombo->currentData().toString();
    ui->hyllyCombo->clear();
    if( !id.isEmpty() ) {
        PilviKysely *kysely = new PilviKysely(pilviModel_, KpKysely::GET, pilviModel_->pilviLoginOsoite() + "/v1/bookshelves");
        kysely->lisaaAttribuutti("officeId", id);
        connect( kysely, &PilviKysely::vastaus, this, &PilveenSiirto::hyllytSaapuu);
        kysely->kysy();

        ui->hyllyCombo->addItem(QIcon(":/pic/talo.png"), ui->toimistoCombo->currentText(), id);
    }
}


void PilveenSiirto::hyllytSaapuu(const QVariant *data)
{    
    for(const auto& item : data->toList()) {
        const auto& map = item.toMap();
        lisaaHylly(map);
    }
    ui->hyllyCombo->model()->sort(0);

    if( ui->hyllyCombo->model()->rowCount() && !kp()->pilvi()->kayttaja().capacity()) {
        ui->toimistoRadio->setChecked(true);
        paivitaInfot();
    }
}

void PilveenSiirto::lisaaHylly(const QVariantMap &hylly)
{
    const auto rights = hylly.value("rights").toList();
    if( rights.contains("OB")) {
        // Oikeus lisätä tähän hyllyyn
        const auto id = hylly.value("id").toString();
        if( ui->toimistoCombo->findData(id) < 0) {
            const auto name = hylly.value("name").toString();
            ui->hyllyCombo->addItem(QIcon(":/pic/arkisto64.png"),name, id);
        }
    }

    for( const auto& sub : hylly.value("subgroups").toList()) {
        const auto group = sub.toMap();
        lisaaHylly(group);
    }
}

void PilveenSiirto::paivitaInfot()
{
    if( (ui->sijaintiGroup->isHidden() ||  ui->omaRadio->isChecked()) &&
        !(ui->toimistoRadio->isVisible() && ui->toimistoRadio->isChecked()) ) {
        ui->infoLabel->show();
        int pilvia = kp()->pilvi()->kayttaja().cloudCount();
        if( pilvia >= kp()->pilvi()->kayttaja().capacity() ) {
            qInfo() << " Kapasiteetti " << pilvia << " / " << kp()->pilvi()->kayttaja().capacity() << " ei riitä pilveen siirtämiseen \n";
            if( kp()->pilvi()->kayttaja().planId()) {
                ui->infoLabel->setText(tr("Kirjanpidon tallentamisesta pilveen veloitetaan %1/kk").arg( kp()->pilvi()->kayttaja().extraMonthly().display() ));
                ui->buttonBox->button(QDialogButtonBox::Ok)->setEnabled(true);
            } else {
                ui->infoLabel->setText(tr("Sinun pitää päivittää tilauksesi ennen kuin voit kopioida tämän kirjanpidon pilveen."));
                ui->infoLabel->setStyleSheet("color: red; font-weight: bold;");
                ui->buttonBox->button(QDialogButtonBox::Ok)->setEnabled(false);
            }
        }
    } else {
        ui->infoLabel->hide();
        ui->buttonBox->button(QDialogButtonBox::Ok)->setEnabled(true);
    }
}



