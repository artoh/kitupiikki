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
#include "vastikelaskutus.h"
#include "ui_vastikelaskutus.h"
#include "db/tositetyyppimodel.h"
#include "db/kirjanpito.h"
#include "rekisteri/maamodel.h"
#include "laskutus/tuotemodel.h"
#include "model/tositerivit.h"
#include "laskutus/laskudlg/rivivientigeneroija.h"
#include "laskutus/tulostus/laskuntulostaja.h"

#include <QPushButton>
#include <QProgressDialog>

VastikeLaskutus::VastikeLaskutus(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::VastikeLaskutus)
{
    ui->setupUi(this);

    QDate date = QDate::currentDate();
    date = date.addMonths(1);
    date = date.addDays( 1 - date.day() );
    ui->jaksoAlkaa->setDate(date);
    ui->jaksoLoppuu->setDate( date.addMonths(12).addDays(-1) );

    connect( &tosite_, &Tosite::tositeTallennettu, this, &VastikeLaskutus::tallennettu);
}

void VastikeLaskutus::laskuta(const QList<int> &huoneistot)
{
    jono_ << huoneistot;
    progress_ = new QProgressDialog(tr("Luodaan laskuja"),
                                    tr("Peru"),
                                    0, huoneistot.count() + 1);
    laskutaSeuraava();
}

VastikeLaskutus::~VastikeLaskutus()
{
    delete ui;
}

void VastikeLaskutus::laskutaSeuraava()
{
    progress_->setValue( progress_->value() + 1 );
    if( jono_.isEmpty()) {
        emit kp()->onni(tr("Uudet laskut löytyvät Lähetettävät-välilehdeltä."));
        progress_->hide();
        deleteLater();
    } else if( progress_->wasCanceled()) {
        deleteLater();
    } else {
        int huoneistoId = jono_.dequeue();
        KpKysely* kysely = kpk(QString("/huoneistot/%1").arg(huoneistoId));
        connect(kysely, &KpKysely::vastaus, this, &VastikeLaskutus::laskutaHuoneisto);
        kysely->kysy();
    }
}

void VastikeLaskutus::laskutaHuoneisto(QVariant *data)
{
    huoneisto_ = data->toMap();
    int asiakas = huoneisto_.value("asiakas").toInt();

    if( asiakas ) {
        KpKysely* kysely = kpk(QString("/kumppanit/%1").arg(asiakas));
        connect( kysely, &KpKysely::vastaus, this, &VastikeLaskutus::asiakasSaapuu);
        kysely->kysy();
    } else {
        laskutaSeuraava();
    }
}

void VastikeLaskutus::asiakasSaapuu(QVariant *data)
{
    alustaTosite();
    asetaAsiakas( data->toMap());
    lisaaTuotteet();

    tosite_.tallennaLiitteitta(Tosite::VALMISLASKU);
}

void VastikeLaskutus::alustaTosite()
{
    tosite_.nollaa( ui->laskuPvm->date(), TositeTyyppi::MYYNTILASKU );
    tosite_.lasku().setLaskunpaiva( ui->laskuPvm->date() );
    tosite_.lasku().setMaksutapa(Lasku::KUUKAUSITTAINEN);
    tosite_.lasku().setValvonta(Lasku::HUONEISTO);
    tosite_.asetaOtsikko( ui->otsikko->text() );
    tosite_.lasku().setOtsikko( ui->otsikko->text());
    tosite_.lasku().setLisatiedot( ui->lisaTiedot->toPlainText());
    tosite_.lasku().setSaate(ui->saate->toPlainText());

    tosite_.lasku().setToimituspvm( ui->jaksoAlkaa->date() );
    tosite_.lasku().setJaksopvm( ui->jaksoLoppuu->date());
    tosite_.lasku().setToistuvanErapaiva(ui->eraPvm->value());

    ViiteNumero viite(ViiteNumero::HUONEISTO, huoneisto_.value("id").toInt());
    tosite_.asetaViite(viite);
    tosite_.lasku().setViite(viite);

    tosite_.lasku().setViivastyskorko( kp()->asetukset()->asetus(AsetusModel::LaskuPeruskorko).toDouble() + 7.0 );


}

void VastikeLaskutus::asetaAsiakas(const QVariantMap &map)
{
    tosite_.asetaKumppani(map);
    tosite_.lasku().setKieli( map.value("kieli").toString() );
    tosite_.lasku().setOsoite( MaaModel::instanssi()->muotoiltuOsoite(map) );
    tosite_.lasku().setEmail( map.value("email").toString());
    tosite_.lasku().setLahetystapa( map.value("laskutapa").toInt());
}

void VastikeLaskutus::lisaaTuotteet()
{
    const QVariantList laskutettavat = huoneisto_.value("laskutus").toList();
    for(const auto& item: laskutettavat) {
        QVariantMap map = item.toMap();
        const QString& lkm = map.value("lkm").toString();
        int tuoteId = map.value("tuote").toInt();

        Tuote tuote = kp()->tuotteet()->tuote(tuoteId);
        tosite_.rivit()->lisaaTuote(tuote, lkm);
    }
    RiviVientiGeneroija riviGeneroija(kp());
    riviGeneroija.generoiViennit(&tosite_);
}

void VastikeLaskutus::tallennettu(QVariant *data)
{
    Tosite tallennettu;
    tallennettu.lataaData(data);

    LaskunTulostaja *tulostaja = new LaskunTulostaja(kp());
    connect( tulostaja, &LaskunTulostaja::laskuLiiteTallennettu,
             this, &VastikeLaskutus::laskutaSeuraava);
    tulostaja->tallennaLaskuLiite(tallennettu);
}
