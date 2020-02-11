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
    setAttribute(Qt::WA_DeleteOnClose);

    ui->setupUi(this);
    alustaAlkusivu();

    connect( ui->buttonBox->button(QDialogButtonBox::Close), &QPushButton::clicked, this, &PilveenSiirto::close);
    connect( ui->buttonBox->button(QDialogButtonBox::Ok), &QPushButton::clicked, this, &PilveenSiirto::accept);
}

PilveenSiirto::~PilveenSiirto()
{
    delete ui;
}

void PilveenSiirto::accept()
{
    ui->rasti1->hide();
    ui->rasti2->hide();
    ui->stackedWidget->setCurrentIndex(KAYNNISSA);
    ui->buttonBox->button(QDialogButtonBox::Ok)->setEnabled(false);
    ui->progressBar->setRange(0, tositelkm_ + liitelkm_ + 50);
    ui->progressBar->setValue(1);

    KpKysely *init = kpk("/init");
    connect( init, &KpKysely::vastaus, this, &PilveenSiirto::initSaapuu);
    init->kysy();
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
    } else if( kp()->pilvi()->plan() == 0) {
        if( tositelkm_ >= 50) {
            ui->infoLabel->setText(tr("Voidaksesi sijoittaa pilveen kirjanpidon, jossa on enemmän kuin 50 "
                                      "tositetta, sinun pitää ensin tehdä Kitsaasta maksullinen tilaus."));
            ui->buttonBox->button(QDialogButtonBox::Ok)->setEnabled(false);
        } else {
            ui->infoLabel->setText(tr("Käytössäsi on Kitsaan maksuton tilaus, joka on tarkoitettu pilven "
                                      "kokeilemiseen. Voit tallentaa kirjanpitoosi enää %1 tositetta lisää "
                                      "ennen kuin sinun on tehtävä maksullinen tilaus.").arg(50 - tositelkm_));
        }
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
    KpKysely *kaverikysely = kpk(QString("/kumppanit/%1").arg(id));
    connect(kaverikysely, &KpKysely::vastaus, this, &PilveenSiirto::tallennaKumppani);
    kaverikysely->kysy();
}

void PilveenSiirto::tallennaKumppani(QVariant *data)
{
    QVariantMap map = data->toMap();
    int id = map.take("id").toInt();

    qDebug() << " tallenna kumppani " << map.value("nimi").toString();

    PilviKysely *tallennus = new PilviKysely(pilviModel_, KpKysely::PUT, QString("/kumppanit/%1").arg(id));
    connect(tallennus, &KpKysely::vastaus, this, &PilveenSiirto::kysySeuraavaKumppani);
    tallennus->kysy(map);
}

void PilveenSiirto::haeTositeLista()
{
    ui->rasti1->show();
    ui->vaihe2->setEnabled(true);

    KpKysely *tositteet = kpk("/tositteet");
    connect( tositteet, &KpKysely::vastaus, this, &PilveenSiirto::tositeListaSaapuu);
    tositteet->kysy();
}

void PilveenSiirto::tositeListaSaapuu(QVariant *data)
{
    const QVariantList& lista = data->toList();
    for(auto item:lista) {
        QVariantMap map = item.toMap();
        int id = map.value("id").toInt();
        tositteet.enqueue(id);
    }
}

void PilveenSiirto::kysySeuraavaTosite()
{
    // TODO!!!!!! Pystyttävä hakemaan KAIKKI tositteet (myös luonnokset)
    if( tositteet.isEmpty()) {
        return;
    }
    int id = tositteet.dequeue();
    KpKysely *kysely = kpk(QString("/tositteet/%1").arg(id));
    connect(kysely, &KpKysely::vastaus, this, &PilveenSiirto::tallennaTosite);
}

void PilveenSiirto::tallennaTosite(QVariant *data)
{
    QVariantMap map = data->toMap();
    int id = map.take("id").toInt();

    qDebug() << "Tosite " << id;

    PilviKysely *tallennus = new PilviKysely(pilviModel_, KpKysely::PUT, QString("/tositteet/%1").arg(id));
    connect(tallennus, &KpKysely::vastaus, this, &PilveenSiirto::kysySeuraavaTosite);
    tallennus->kysy();
    ui->progressBar->setValue( ui->progressBar->value() + 1);
}

