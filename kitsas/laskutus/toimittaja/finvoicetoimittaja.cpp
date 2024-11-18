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
#include "finvoicetoimittaja.h"
#include "db/kirjanpito.h"
#include "rekisteri/asiakastoimittajadlg.h"
#include "pilvi/pilvikysely.h"
#include "maaritys/verkkolasku/verkkolaskumaaritys.h"
#include <QSettings>
#include "model/lasku.h"
#include "db/tositetyyppimodel.h"
#include "model/tositerivi.h"
#include "rekisteri/maamodel.h"
#include "liite/liitteetmodel.h"
#include "model/tositerivit.h"
#include "model/tositerivi.h"
#include "laskutus/tulostus/laskuntulostaja.h"

#include "model/toiminimimodel.h"

FinvoiceToimittaja::FinvoiceToimittaja(QObject *parent) :
    AbstraktiToimittaja(parent)
{

}

void FinvoiceToimittaja::toimita()
{
    if( !kp()->asetukset()->luku("FinvoiceKaytossa") ) {
        virhe(tr("Verkkolaskutusta ei ole määritelty käyttöön kirjanpidon asetuksista"));
    } else if(!kp()->pilvi()->kayttajaPilvessa()) {
        virhe(tr("Verkkolaskujen toimittaminen edellyttää kirjautumista Kitsaan pilveen"));
    } else if( kp()->asetukset()->asetus(AsetusModel::LaskuIbanit).isEmpty() ) {
        virhe(tr("Laskutuksen asetuksissa ei ole määritelty yhtään tilinumeroa."));
    } else {
        if( init_.isEmpty() || init_.value("ovt") != kp()->asetukset()->asetus(AsetusModel::OvtTunnnus))    // Varmistetaan, ettei kirjanpito vaihtunut
            alustaInit();
        if( kp()->asetukset()->luku("FinvoiceKaytossa") == VerkkolaskuMaaritys::PAIKALLINEN) {
           paikallinenVerkkolasku();
        } else if( kp()->asetukset()->luku("FinvoiceKaytossa") == VerkkolaskuMaaritys::MAVENTA) {
            aloitaMaventa();
        }
    }
}

void FinvoiceToimittaja::alustaInit()
{
    const AsetusModel* asetukset = kp()->asetukset();

    init_.insert("ovt", asetukset->asetus(AsetusModel::OvtTunnnus));
    init_.insert("operaattori", asetukset->asetus(AsetusModel::Operaattori));
    init_.insert("nimi", asetukset->asetus(AsetusModel::OrganisaatioNimi));
    init_.insert("alvtunnus", AsiakasToimittajaDlg::yToAlv( asetukset->asetus(AsetusModel::Ytunnus) ) );
    init_.insert("kotipaikka", asetukset->asetus(AsetusModel::Kotipaikka));
    init_.insert("osoite", asetukset->asetus(AsetusModel::Katuosoite));
    init_.insert("postinumero", asetukset->asetus(AsetusModel::Postinumero));
    init_.insert("kaupunki", asetukset->asetus(AsetusModel::Kaupunki));
    init_.insert("unitdecimals", kp()->asetukset()->luku("LaskuYksikkoDesimaalit", 2));

    QVariantList tilit;
    for(const QString& ibanStr : asetukset->asetus(AsetusModel::LaskuIbanit).split(',')) {
        Iban iban(ibanStr);
        QVariantMap tili;

        tili.insert("iban", iban.valeitta());
        const Tili& haettuTili = kp()->tilit()->tiliIbanilla(iban.valeitta());
        if( haettuTili.onkoValidi() && !haettuTili.bic().isEmpty()) {
            tili.insert("bic", haettuTili.bic());
        } else {
            tili.insert("bic", iban.bic());
        }
        tilit.append(tili);
    }
    init_.insert("tilit", tilit);
}

void FinvoiceToimittaja::paikallinenVerkkolasku()
{
    QString osoite = kp()->pilvi()->finvoiceOsoite() + "/create";
    PilviKysely *pk = new PilviKysely( kp()->pilvi(), KpKysely::POST,
                osoite );
    if( kp()->asetukset()->onko("FinvoiceSOAP")) pk->lisaaAttribuutti("soap");
    connect( pk, &PilviKysely::vastaus, this, &FinvoiceToimittaja::laskuSaapuu);
    connect( pk, &KpKysely::virhe, this, [this] {  this->virhe(tr("Verkkolaskun muodostaminen epäonnistui"));} );

    pk->kysy(finvoiceJson());

}


void FinvoiceToimittaja::laskuSaapuu(QVariant *data)
{
    QByteArray lasku = data->toByteArray();

    QString hakemisto = kp()->settings()->value( QString("FinvoiceHakemisto/%1").arg(kp()->asetukset()->asetus(AsetusModel::UID))).toString();
    QString tnimi = QString("%1/lasku%2.xml")
            .arg(hakemisto)
            .arg(tosite()->lasku().numero().toULong(),8,10,QChar('0'));
    QFile out(tnimi);
    if( !out.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        virhe(tr("Laskutiedoston tallentaminen sijaintiin %1 epäonnistui").arg(hakemisto));
    } else {
        out.write(lasku);
        out.close();
    }
    valmis();
}


QVariantMap FinvoiceToimittaja::finvoiceJson()
{
    QVariantMap asiakas = tosite()->kumppanimap();
    if( (asiakas.value("ovt").toString().isEmpty() || asiakas.value("operaattori").toString().isEmpty()) &&
         tosite()->lasku().lahetystapa() != Lasku::POSTITUS ) {
        virhe(tr("Verkkolaskun saajalle ei ole määritelty verkkolaskutusosoitetta"));
        return QVariantMap();
    }

    MaaModel::Maa maa = MaaModel::instanssi()->maaKoodilla(asiakas.value("maa").toString());
    asiakas.insert("maanimi", maa.englanniksi());

    QVariantMap pyynto;
    QVariantMap init(init_);
    Lasku lasku = tosite()->lasku();

    if( lasku.toiminimi() ) {
        init.insert("aputoiminimi", kp()->toiminimet()->tieto(ToiminimiModel::Nimi, lasku.toiminimi() ));
    }

    pyynto.insert("init", init);
    pyynto.insert("asiakas", asiakas);

    int tyyppi = tosite()->tyyppi();


    if( lasku.maksutapa() == Lasku::KATEINEN  || lasku.maksutapa() == Lasku::KORTTIMAKSU)
        lasku.set("tyyppi", "KUITTI");
    else if( tyyppi == TositeTyyppi::HYVITYSLASKU)
        lasku.set("tyyppi", "HYVITYSLASKU");
    else if( tyyppi == TositeTyyppi::MAKSUMUISTUTUS)
        lasku.set("tyyppi", "MAKSUMUISTUTUS");
    else if( lasku.maksutapa() == Lasku::ENNAKKOLASKU)
        lasku.set("tyyppi", "ENNAKKOLASKU");
    else
        lasku.set("tyyppi", "LASKU");

    pyynto.insert("lasku", lasku.data() );


    for( int i = 0; i < tosite()->rivit()->rowCount(); i++ ) {
        TositeRivi rivi = tosite()->rivit()->rivi(i);
        if( !rivi.unKoodi().isEmpty() ) {
            rivi.setYksikko( kp()->kaanna("UN_" + rivi.unKoodi(), lasku.kieli().toLower()) );
            tosite()->rivit()->asetaRivi(i, rivi);
        }
    }

    pyynto.insert("rivit", tosite()->rivit()->rivit());
    pyynto.insert("docid", tosite()->id());

    return pyynto;

}

void FinvoiceToimittaja::aloitaMaventa()
{
    liitteet_.clear();
    liiteIndeksi_ = -1;

    LaskunTulostaja tulostaja(kp());    
    QByteArray kuva = tulostaja.pdf(*tosite());

    QString osoite = kp()->pilvi()->finvoiceOsoite() + "/attachment";
    PilviKysely *pk = new PilviKysely( kp()->pilvi(), KpKysely::POST,
                osoite );
    connect( pk, &PilviKysely::vastaus, this, &FinvoiceToimittaja::liiteLiitetty);
    connect( pk, &PilviKysely::virhe, [this] { this->virhe(tr("Verkkolaskun kuvan liittäminen epäonnistui")); });
    pk->lahetaTiedosto(kuva);

}

void FinvoiceToimittaja::liiteLiitetty(QVariant *data)
{
    // Liitetään saapunut
    QVariantMap map = data->toMap();
    if( liiteIndeksi_ < 0) {
        map.insert("nimi", tosite()->laskuNumero() + ".pdf");
    } else {
        const QModelIndex index = tosite()->liitteet()->index(liiteIndeksi_);
        map.insert("nimi", index.data(LiitteetModel::NimiRooli).toString());
    }
    liitteet_.append(map);

    // Siirrytään seuraavaan
    liiteIndeksi_++;

    // Hypätään laskun kuvan yli
    if( tosite()->liitteet()->index(liiteIndeksi_).data(LiitteetModel::RooliRooli).toString() == "lasku") {
        liiteIndeksi_++;
    }

    // Jos liitteitä on jäljellä, liitetään seuraava liite, muuten siirrytään
    // eteenpäin
    if( liiteIndeksi_ < tosite()->liitteet()->rowCount()) {
        const QByteArray& sisalto = tosite()->liitteet()->index(liiteIndeksi_).data(LiitteetModel::SisaltoRooli).toByteArray();
        QString osoite = kp()->pilvi()->finvoiceOsoite() + "/attachment";
        PilviKysely *pk = new PilviKysely( kp()->pilvi(), KpKysely::POST,
                    osoite );
        connect( pk, &PilviKysely::vastaus, this, &FinvoiceToimittaja::liiteLiitetty);
        connect( pk, &PilviKysely::virhe, [this] { this->virhe(tr("Liitteen liittäminen epäonnistui")); });
        pk->lahetaTiedosto(sisalto);

    } else {
        lahetaMaventa();
    }
}

void FinvoiceToimittaja::lahetaMaventa()
{
    QVariantMap pyynto = finvoiceJson();
    pyynto.insert("liitteet", liitteet_);

    QString osoite = kp()->pilvi()->finvoiceOsoite() + "/invoices/" + kp()->asetukset()->asetus(AsetusModel::Ytunnus);

    PilviKysely *pk = new PilviKysely( kp()->pilvi(), KpKysely::POST,
                osoite );
    connect( pk, &PilviKysely::vastaus, this, &FinvoiceToimittaja::maventaToimitettu);
    connect( pk, &KpKysely::virhe, this, &FinvoiceToimittaja::maventaVirhe);

    pk->kysy(pyynto);

}

void FinvoiceToimittaja::maventaToimitettu(QVariant *data)
{
    QVariantMap tulos = data->toMap();
    QString maventaId = tulos.value("id").toString();

    if( maventaId.length() > 4 ) {

        tosite()->setData(Tosite::MAVENTAID, maventaId);

        connect( tosite(), &Tosite::talletettu, this, [this] { this->valmis(true);});
        connect( tosite(), &Tosite::tallennusvirhe, this, [this] { this->virhe(tr("Lähetetyksi merkitseminen epäonnistui"));});
        tosite()->tallenna(Tosite::LAHETETTYLASKU);

    } else {
        virhe(tr("Verkkolaskun lähettäminen Maventan palveluun epäonnistui. Tarkasta verkkolaskutuksen asetukset"));
    }  
}

void FinvoiceToimittaja::maventaVirhe(int koodi, const QString &selitys)
{
    if( koodi == 201 ) {
        virhe(tr("Käyttäjää ei ole yhdistetty Maventan tunnukseen. Määritä käyttäjä verkkolaskutuksen asetuksista."));
    } else {
        this->virhe(selitys);
    }

}

