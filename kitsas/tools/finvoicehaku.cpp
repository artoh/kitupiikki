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
#include "finvoicehaku.h"

#include "db/kirjanpito.h"
#include "pilvi/pilvimodel.h"
#include "pilvi/pilvikysely.h"

#include "maaritys/verkkolasku/verkkolaskumaaritys.h"

#include "model/tosite.h"
#include "model/tositeviennit.h"
#include "model/tositeliitteet.h"
#include "model/tositevienti.h"
#include "db/tositetyyppimodel.h"
#include "db/tilityyppimodel.h"

#include <QDebug>



FinvoiceHaku::FinvoiceHaku(QObject *parent) : QObject(parent)
{

}


FinvoiceHaku *FinvoiceHaku::init(Kirjanpito *kp)
{
    instanssi__ = new FinvoiceHaku(kp);
    return instanssi__;
}

void FinvoiceHaku::haeUudet()
{
    if( kp()->yhteysModel() && kp()->asetukset()->luku("FinvoiceKaytossa") == VerkkolaskuMaaritys::MAVENTA &&
            kp()->pilvi()->kayttajaPilvessa()
            && !hakuPaalla_
            && !qobject_cast<PilviModel*>(kp()->yhteysModel())) {
        // Haetaan uudet laskut, jos on paikallinen kirjanpito ja siinä Finvoice
        haettuLkm_ = 0;
        hakuPaalla_ = true;
        ytunnus_ = kp()->asetukset()->ytunnus().simplified();
        aikaleima_ = QDateTime();

        QString osoite = kp()->pilvi()->finvoiceOsoite() + "/invoices/" + ytunnus_;
        PilviKysely *haku = new PilviKysely( kp()->pilvi(), PilviKysely::GET, osoite);
        connect( haku, &KpKysely::vastaus, this, &FinvoiceHaku::listaSaapuu);
        connect( haku, &KpKysely::virhe, [this] { this->hakuPaalla_=false;});
        haku->kysy();

        // Haetaan myös statustiedot
        QString statusosoite = kp()->pilvi()->finvoiceOsoite() + "/status/" + ytunnus_;
        PilviKysely *statushaku = new PilviKysely( kp()->pilvi(), PilviKysely::GET, statusosoite);
        connect( statushaku, &KpKysely::vastaus, this, &FinvoiceHaku::statusListaSaapuu);
        statushaku->kysy();
    }
}

void FinvoiceHaku::listaSaapuu(QVariant *data)
{
    hakulista_ = data->toList();
    if( !hakulista_.isEmpty() ) {
        emit kp()->onni(tr("Noudetaan uusia verkkolaskuja"), Kirjanpito::Haetaan);
        haeSeuraava();
    } else {
        hakuPaalla_ = false;
    }
}

void FinvoiceHaku::haeSeuraava()
{

    nykyTosite_ = new Tosite(this);
    nykyinen_ = hakulista_.takeFirst().toMap();

    nykyTosite_->asetaTyyppi(TositeTyyppi::SAAPUNUTVERKKOLASKU);    

    QString osoite = kp()->pilvi()->finvoiceOsoite() + "/invoices/" + ytunnus_ + "/" + nykyinen_.value("id").toString();
    PilviKysely *haku = new PilviKysely( kp()->pilvi(), PilviKysely::GET, osoite);
    haku->lisaaAttribuutti("format","JSON");
    connect( haku, &KpKysely::vastaus, this, &FinvoiceHaku::jsonSaapuu);
    haku->kysy();

}

void FinvoiceHaku::jsonSaapuu(QVariant *data)
{
    QVariantMap json = data->toMap();

    QString alvtunnus = json.value("toimittaja").toMap().value("alvtunnus").toString();
    if( alvtunnus.isEmpty())
        kumppaniSaapuu(nullptr, json);
    else {
        KpKysely *kysely = kpk("/kumppanit");
        kysely->lisaaAttribuutti("alv", alvtunnus);
        connect( kysely, &KpKysely::vastaus,  [this, json] (QVariant* data) { this->kumppaniSaapuu(data, json); });
        kysely->kysy();
    }
}

void FinvoiceHaku::kumppaniSaapuu(QVariant *data, const QVariantMap json)
{
    QVariantMap kmap;
    if( data && data->toList().count()) {
        kmap = data->toList().value(0).toMap();
        nykyTosite_->asetaKumppani(kmap.value("id").toInt() );
    } else {
        nykyTosite_->asetaKumppani(json.value("toimittaja").toMap());
    }

    QVariantMap lasku = json.value("lasku").toMap();
    QDate pvm = lasku.value("pvm").toDate();

    nykyTosite_->asetaPvm(pvm);
    nykyTosite_->asetaLaskupvm(pvm);
    nykyTosite_->asetaErapvm(lasku.value("erapvm").toDate());
    nykyTosite_->asetaViite(lasku.value("viite").toString());

    TositeVienti vasta;
    vasta.setPvm(pvm);
    vasta.setTyyppi(TositeVienti::OSTO + TositeVienti::VASTAKIRJAUS);
    vasta.setEra(-1);
    vasta.setKredit(lasku.value("summa").toDouble());
    vasta.setTili(kp()->asetukset()->luku("Ostovelkatili", 2960));
    nykyTosite_->viennit()->lisaa(vasta);

    QVariantList alvit = json.value("alv").toList();
    for(auto& alvi : alvit) {
        QVariantMap alvmap = alvi.toMap();
        TositeVienti netto;
        netto.setPvm(pvm);
        netto.setDebet(alvmap.value("netto").toDouble());
        netto.setAlvKoodi( alvmap.value("alvkoodi").toInt());
        netto.setAlvProsentti( alvmap.value("alvprosentti").toDouble());
        netto.setTili( kmap.value("menotili", kp()->asetukset()->asetus(AsetusModel::OletusMenotili)).toInt() );
        netto.setTyyppi( TositeVienti::OSTO + TositeVienti::KIRJAUS );
        nykyTosite_->viennit()->lisaa(netto);

        if( qAbs(alvmap.value("vero").toDouble()) > 1e-5) {
            TositeVienti vero;
            vero.setPvm(pvm);
            vero.setDebet(alvmap.value("vero").toDouble());
            vero.setAlvKoodi( alvmap.value("alvkoodi").toInt());
            vero.setAlvProsentti( alvmap.value("alvprosentti").toDouble());
            vero.setTili(kp()->tilit()->tiliTyypilla(TiliLaji::ALVVELKA).numero());
            vero.setTyyppi(TositeVienti::OSTO + TositeVienti::ALVKIRJAUS);
            nykyTosite_->viennit()->lisaa(vero);
        }
    }
    QVariantMap ostolasku;
    ostolasku.insert("pvm", pvm);
    ostolasku.insert("numero", lasku.value("numero"));
    ostolasku.insert("iban", lasku.value("iban"));
    ostolasku.insert("maventaid", nykyinen_.value("id"));

    QString osoite = kp()->pilvi()->finvoiceOsoite() + "/invoices/" + ytunnus_ + "/" + nykyinen_.value("id").toString();
    PilviKysely *haku = new PilviKysely( kp()->pilvi(), PilviKysely::GET, osoite);
    haku->lisaaAttribuutti("format","ORIGINAL_OR_GENERATED_IMAGE");
    connect( haku, &KpKysely::vastaus, this, &FinvoiceHaku::kuvaSaapuu);
    haku->kysy();

}

void FinvoiceHaku::kuvaSaapuu(QVariant *data)
{
    nykyTosite_->liitteet()->lisaa(data->toByteArray(),"lasku.pdf");

    QString osoite = kp()->pilvi()->finvoiceOsoite() + "/invoices/" + ytunnus_ + "/" + nykyinen_.value("id").toString();
    PilviKysely *haku = new PilviKysely( kp()->pilvi(), PilviKysely::GET, osoite);
    haku->lisaaAttribuutti("format","XML");
    connect( haku, &KpKysely::vastaus, this, &FinvoiceHaku::xmlSaapuu);
    haku->kysy();
}

void FinvoiceHaku::xmlSaapuu(QVariant *data)
{
    if( ytunnus_ != kp()->asetukset()->ytunnus() ) {
        hakulista_.clear();
        hakuPaalla_ = false;
        return;
    }
    connect( nykyTosite_, &Tosite::talletettu, this, &FinvoiceHaku::tallennettu );

    nykyTosite_->liitteet()->lisaa(data->toByteArray(),"lasku.xml");
    nykyTosite_->tallenna(Tosite::SAAPUNUT);
}

void FinvoiceHaku::tallennettu()
{
    haettuLkm_++;
    QDateTime aikaleima = nykyinen_.value("received_at").toDateTime();
    if( aikaleima_.isNull() || aikaleima > aikaleima_)
        aikaleima_ = aikaleima.addSecs(1);

    if( !hakulista_.isEmpty())
        haeSeuraava();
    else {
        emit kp()->onni(tr("%1 verkkolaskua haettu").arg(haettuLkm_));
        hakuPaalla_ = false;

        // Ilmoitetaan, että haettu on
        QString osoite = kp()->pilvi()->finvoiceOsoite() + "/invoices/" + ytunnus_;
        PilviKysely *haku = new PilviKysely( kp()->pilvi(), PilviKysely::PUT, osoite);
        haku->lisaaAttribuutti("lasttime",aikaleima_.toString(Qt::ISODate));
        haku->kysy();

    }

}

void FinvoiceHaku::statusListaSaapuu(QVariant *data)
{
    statusLista_ = data->toList();
    if(statusLista_.count())
        seuraavaStatus();
}

void FinvoiceHaku::seuraavaStatus()
{
    if( statusLista_.count()) {
        QVariant kasiteltava = statusLista_.takeLast();
        QVariantMap map = kasiteltava.toMap();

        int tositeId = map.value("docid").toInt();
        int status = map.value("status").toInt();

        KpKysely *kysely = kpk(QString("/tositteet/%1").arg(tositeId), KpKysely::PATCH);
        if(kysely) {
            QVariantMap data;
            if(status == 1) {
                data.insert("tila", Tosite::TOIMITETTULASKU);
            } else if( status == 2) {
                data.insert("tila", Tosite::LAHETYSVIRHE);
            }
            connect(kysely, &KpKysely::vastaus, this, &FinvoiceHaku::seuraavaStatus);
            kysely->kysy(data);
        }
    }
}

FinvoiceHaku *FinvoiceHaku::instanssi()
{
   return instanssi__;
}

FinvoiceHaku* FinvoiceHaku::instanssi__ = nullptr;
