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

#include <QSqlQuery>
#include <QImage>
#include <QPixmap>
#include <QPushButton>
#include <QMessageBox>
#include <QDebug>

PilveenSiirto::PilveenSiirto(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::PilveenSiirto),
    pilviModel_(new PilviModel(this, kp()->pilvi()->userToken()))
{

    ui->setupUi(this);
    alustaAlkusivu();

    connect( ui->buttonBox->button(QDialogButtonBox::Cancel), &QPushButton::clicked, this, &PilveenSiirto::close);
    connect( ui->buttonBox->button(QDialogButtonBox::Ok), &QPushButton::clicked, this, &PilveenSiirto::accept);
}

PilveenSiirto::~PilveenSiirto()
{
    delete ui;
}

void PilveenSiirto::accept()
{
    if( pilviId_) {
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
    ui->nimiLabel->setText( kp()->asetus("Nimi") );
    if( !kp()->logo().isNull())
        ui->logoLabel->setPixmap( QPixmap::fromImage( kp()->logo().scaled(32,32,Qt::KeepAspectRatio)) );

    int pilvia = kp()->pilvi()->omatPilvet();
    int pilvetMax = kp()->pilvi()->pilviMax();

    QSqlQuery kysely(kp()->sqlite()->tietokanta());
    kysely.exec("SELECT COUNT(id) FROM Tosite");
    kysely.next();
    tositelkm_ = kysely.value(0).toInt();

    kysely.exec("SELECT COUNT(id) FROM Liite");
    kysely.next();
    liitelkm_ = kysely.value(0).toInt();

    if( pilvia >= pilvetMax) {
        ui->infoLabel->setText(tr("Nykyiseen tilaukseesi kuuluu %1 pilvessä olevaa kirjanpitoa.\n"
                                  "Sinun pitää päivittää tilauksesi ennen kuin voit kopioida tämän kirjanpidon pilveen.").arg(pilvetMax));
        ui->buttonBox->button(QDialogButtonBox::Ok)->setEnabled(false);
    }
}



void PilveenSiirto::initSaapuu(QVariant *data)
{
    QVariantMap init = data->toMap();

    // Poistetaan Yleinen-kohdennus, joka on valmiina tietokannassa
    QVariantList kohdennukset = init.value("kohdennukset").toList();
    kohdennukset.removeFirst();
    init.insert("kohdennukset", kohdennukset);

    QVariantMap map;
    map.insert("name", kp()->asetus("Nimi"));
    map.insert("businessid", kp()->asetus("Ytunnus"));
    map.insert("trial", kp()->onkoHarjoitus());
    map.insert("init", init);

    qDebug() << "token " << pilviModel_->token();

    PilviKysely* kysely = new PilviKysely(pilviModel_, KpKysely::POST, pilviModel_->pilviLoginOsoite() + "/clouds");
    connect( kysely, &PilviKysely::vastaus, this, &PilveenSiirto::pilviLuotu);
    kysely->kysy(map);
    ui->progressBar->setValue(10);

    for(auto tk : init.value("tilikaudet").toList()) {
        QVariantMap tkmap = tk.toMap();
        tilikaudet.append( tkmap.value("alkaa").toString());
    }
}

void PilveenSiirto::pilviLuotu(QVariant *data)
{
    const QVariantMap& map = data->toMap();
    pilviId_ = map.value("id").toInt();

    qDebug() << "Pilvi luotu " << map.value("id");
    qDebug() << pilviModel_->pilviosoite();

    connect( pilviModel_, &PilviModel::kirjauduttu, this, &PilveenSiirto::avaaLuotuPilvi);    
    pilviModel_->paivitaLista();
    ui->progressBar->setValue(20);
}

void PilveenSiirto::avaaLuotuPilvi()
{
    qDebug() << "Avataan luotu pilvi";
    pilviModel_->avaaPilvesta(pilviId_, true);
    qDebug() << pilviModel_->pilviosoite();

    ui->progressBar->setValue(30);
    haeRyhmaLista();
    qDebug() << kp()->yhteysModel();

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
    tallennus->kysy(map);
}

void PilveenSiirto::haeKumppaniLista()
{
    qDebug() << "Hae kumppanilista";
    KpKysely *kaverikysely = kpk("/kumppanit");
    connect( kaverikysely, &KpKysely::vastaus, this, &PilveenSiirto::kumppaniListaSaapuu);
    kaverikysely->kysy();
}

void PilveenSiirto::kumppaniListaSaapuu(QVariant *data)
{
    const QVariantList& lista = data->toList();
    for( auto item : lista) {
        QVariantMap map = item.toMap();
        int id = map.value("id").toInt();
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
    qDebug() << "Kumppani " << id << " jäljellä " << kumppanit.count();
    KpKysely *kaverikysely = kpk(QString("/kumppanit/%1").arg(id));
    connect(kaverikysely, &KpKysely::vastaus, this, &PilveenSiirto::tallennaKumppani);
    kaverikysely->kysy();
}

void PilveenSiirto::tallennaKumppani(QVariant *data)
{
    QVariantMap map = data->toMap();
    int id = map.take("id").toInt();

    if( map.value("nimi").toString() == "Verohallinto") {
        qDebug() << "Verohallinto";
        kysySeuraavaKumppani();
        return;
    }

    qDebug() << " tallenna kumppani " << map.value("nimi").toString();

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
        QVariantMap laskuMap = map.value("lasku").toMap();
        laskuMap.insert("numero", laskuMap.value("numero").toInt());
        laskuMap.insert("viivkorko", laskuMap.value("viivkorko").toDouble());
        QStringList keys = laskuMap.keys();
        for( auto key : keys) {
            if( laskuMap.value(key).isNull())
                laskuMap.remove(key);
        }
        map.insert("lasku", laskuMap);
    }

    qDebug() << "Tosite " << id << " jaljella " << tositteet.count();

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
    if( liitekysely.next()) {
        QString nimi = liitekysely.value("nimi").toString();
        QString tyyppi = liitekysely.value("tyyppi").toString();
        QString rooli = liitekysely.value("roolinimi").toString();
        QByteArray data = liitekysely.value("data").toByteArray();
        int tosite = liitekysely.value("tosite").toInt();

        QMap<QString,QString> meta;
        meta.insert("Content-type", tyyppi);
        meta.insert("Filename", nimi);

        KpKysely *kysely = rooli.isEmpty() ?
                    new PilviKysely(pilviModel_, KpKysely::POST, QString("/liitteet/%1").arg(tosite)) :
                    new PilviKysely(pilviModel_, KpKysely::PUT, QString("/liitteet/%1/%2").arg(tosite).arg(rooli));
        connect(kysely, &KpKysely::vastaus, this, &PilveenSiirto::tallennaSeuraavaLiite);
        connect(kysely, &KpKysely::virhe, this, &PilveenSiirto::siirtoVirhe);
        kysely->lahetaTiedosto(data, meta);
        ui->progressBar->setValue(ui->progressBar->value()+1);

    } else {
        tallennaBudjetit();
        tallennaVakioviitteet();
    }
}

void PilveenSiirto::tallennaBudjetit()
{
    for(QString kausi : tilikaudet) {
        KpKysely *haku = kpk(QString("/budjetti/%1").arg(kausi));
        haku->lisaaAttribuutti("kohdennukset");
        connect( haku, &KpKysely::vastaus, [this, kausi] (QVariant *data) { this->tallennaBudjetti(kausi, data); } );
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
        map.insert("LaskuNumerointialkaa", kp()->asetukset()->luku("LaskuNumerointialkaa"));
        KpKysely *lst = new PilviKysely(pilviModel_, KpKysely::PATCH, "/asetukset");
        connect( lst, &KpKysely::vastaus, this, &PilveenSiirto::valmis);
        connect(lst, &KpKysely::virhe, this, &PilveenSiirto::siirtoVirhe);
        lst->kysy(map);
        valmis();
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

void PilveenSiirto::valmis()
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

    if( tositteita != tositelkm_) {
        qDebug() << QString("Tositteita siirretty %1 / %2").arg(tositteita).arg(tositelkm_);
        siirtoVirhe(0);
    } else {
        ui->buttonBox->show();
        ui->buttonBox->button(QDialogButtonBox::Cancel)->hide();
        ui->valmisInfo->setText(tr("Tositteita kopioitu %1 kpl, kirjanpidon koko pilvessä %L2 Mt")
                                .arg(tositteita)
                                .arg(koko / 1000000.0 ,0,'f',1 ));
        ui->stackedWidget->setCurrentIndex(VALMIS);
        kp()->pilvi()->paivitaLista(pilviId_);
    }
}

void PilveenSiirto::siirtoVirhe(int koodi)
{
    qDebug() << "Siirtovirhe " << koodi;
    ui->stackedWidget->setCurrentIndex(VALMIS);
    ui->buttonBox->show();
    ui->buttonBox->button(QDialogButtonBox::Cancel)->hide();
    if( koodi == 302)
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



