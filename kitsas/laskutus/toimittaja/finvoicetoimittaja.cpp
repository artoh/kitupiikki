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
    } else {
        if( init_.isEmpty())
            alustaInit();

        QVariantMap asiakas = tositeMap().value("kumppani").toMap();
        if( (asiakas.value("ovt").toString().isEmpty() || asiakas.value("operaattori").toString().isEmpty()) &&
             tositeMap().value("lasku").toMap().value("laskutapa").toInt() != Lasku::POSTITUS ) {
            virhe(tr("Verkkolaskun saajalle ei ole määritelty verkkolaskutusosoitetta"));
            return;
        }

        MaaModel::Maa maa = MaaModel::instanssi()->maaKoodilla(asiakas.value("maa").toString());
        asiakas.insert("maanimi", maa.englanniksi());

        QVariantMap pyynto;
        pyynto.insert("init", init_);
        pyynto.insert("asiakas", asiakas);

        Lasku lasku = tositeMap().value("lasku").toMap();
        int tyyppi = tositeMap().value("tyyppi").toInt();


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

        QVariantList rivit;
        for(const auto& item : tositeMap().value("rivit").toList()) {
            TositeRivi rivi = item.toMap();
            if( !rivi.unKoodi().isEmpty() )
                rivi.setYksikko( kp()->kaanna("UN_" + rivi.unKoodi(), lasku.kieli().toLower()) );
            rivit.append(rivi.data());
        }

        pyynto.insert("rivit", rivit);
        pyynto.insert("docid", tositeMap().value("id").toInt());

        if( kp()->asetukset()->luku("FinvoiceKaytossa") == VerkkolaskuMaaritys::PAIKALLINEN) {

            QString osoite = kp()->pilvi()->finvoiceOsoite() + "/create";
            PilviKysely *pk = new PilviKysely( kp()->pilvi(), KpKysely::POST,
                        osoite );
            if( kp()->asetukset()->onko("FinvoiceSOAP")) pk->lisaaAttribuutti("soap");
            connect( pk, &PilviKysely::vastaus, this, &FinvoiceToimittaja::laskuSaapuu);
            connect( pk, &KpKysely::virhe, this, [this] {  this->virhe(tr("Verkkolaskun muodostaminen epäonnistui"));} );

            pk->kysy(pyynto);

        } else if( kp()->asetukset()->luku("FinvoiceKaytossa") == VerkkolaskuMaaritys::MAVENTA) {
            QString osoite = kp()->pilvi()->finvoiceOsoite() + "/invoices/" + kp()->asetukset()->asetus(AsetusModel::Ytunnus);

            PilviKysely *pk = new PilviKysely( kp()->pilvi(), KpKysely::POST,
                        osoite );
            connect( pk, &PilviKysely::vastaus, this, &FinvoiceToimittaja::maventaToimitettu);
            connect( pk, &KpKysely::virhe, this, &FinvoiceToimittaja::maventaVirhe);

            pk->kysy(pyynto);
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
        tili.insert("bic", iban.bic());
        tilit.append(tili);
    }
    init_.insert("tilit", tilit);
}


void FinvoiceToimittaja::laskuSaapuu(QVariant *data)
{
    QByteArray lasku = data->toByteArray();

    QString hakemisto = kp()->settings()->value( QString("FinvoiceHakemisto/%1").arg(kp()->asetukset()->asetus(AsetusModel::UID))).toString();
    QString tnimi = QString("%1/lasku%2.xml")
            .arg(hakemisto)
            .arg(tositeMap().value("lasku").toMap().value("numero").toLongLong(),8,10,QChar('0'));
    QFile out(tnimi);
    if( !out.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        virhe(tr("Laskutiedoston tallentaminen sijaintiin %1 epäonnistui").arg(hakemisto));
    } else {
        out.write(lasku);
        out.close();
    }
    merkkaaToimitetuksi();
}

void FinvoiceToimittaja::maventaToimitettu(QVariant *data)
{
    QVariantMap tulos = data->toMap();
    QString maventaId = tulos.value("id").toString();

    if( maventaId.length() > 4 ) {

    QVariantMap tosite(tositeMap());
    tosite.insert("maventaid", maventaId);
    tosite.insert("tila", Tosite::LAHETETTYLASKU);

    KpKysely *kysely = kpk(QString("/tositteet/%1").arg(tosite.value("id").toInt()), KpKysely::PUT);
    connect( kysely, &KpKysely::vastaus, this, &FinvoiceToimittaja::valmis);
    kysely->kysy(tosite);

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
