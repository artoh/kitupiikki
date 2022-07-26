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
#include "sahkopostitoimittaja.h"

#include "smtpclient/SmtpMime"
#include "maaritys/emailmaaritys.h"

#include "laskutus/tulostus/laskuntulostaja.h"
#include "db/kirjanpito.h"
#include <QSettings>
#include "db/asetusmodel.h"
#include "db/tositetyyppimodel.h"

#include "model/tositeliitteet.h"
#include "pilvi/pilvikysely.h"

SahkopostiToimittaja::SahkopostiToimittaja(QObject *parent)
    : AbstraktiToimittaja(parent)
{

}


void SahkopostiToimittaja::toimita()
{
    tosite_ = new Tosite(this);
    const QVariantMap& tositteenTiedot = tositeMap();
    tosite_->lataa(tositteenTiedot);

    if( kp()->pilvi() && kp()->pilvi()->tilausvoimassa() && kp()->asetukset()->onko("KitsasEmail")) {
        connect( tosite_->liitteet(), &TositeLiitteet::kaikkiLiitteetHaettu, this, &SahkopostiToimittaja::lahetaKitsas );
    } else {
        connect( tosite_->liitteet(), &TositeLiitteet::kaikkiLiitteetHaettu, this, &SahkopostiToimittaja::laheta );
    }
    tosite_->liitteet()->lataaKaikkiLiitteet();
}

void SahkopostiToimittaja::laheta()
{
    bool kpasetus = !kp()->asetukset()->asetus(AsetusModel::SmtpServer).isEmpty();
    QString server = kpasetus ? kp()->asetukset()->asetus(AsetusModel::SmtpServer) : kp()->settings()->value("SmtpServer").toString();
    int port = kpasetus ? kp()->asetukset()->luku(AsetusModel::SmtpPort) : kp()->settings()->value("SmtpPort").toInt();
    QString user = kpasetus ? kp()->asetukset()->asetus(AsetusModel::SmtpUser) : kp()->settings()->value("SmtpUser").toString();
    QString password = kpasetus ? kp()->asetukset()->asetus(AsetusModel::SmtpPassword) : kp()->settings()->value("SmtpPassword").toString();
    int tyyppi = EmailMaaritys::sslIndeksi( kpasetus ? kp()->asetukset()->asetus(AsetusModel::EmailSSL) : kp()->settings()->value("EmailSSL").toString() );
    QString kenelta = kpasetus ? kp()->asetukset()->asetus(AsetusModel::EmailNimi) : kp()->settings()->value("EmailNimi").toString();
    QString keneltaEmail = kpasetus ? kp()->asetukset()->asetus(AsetusModel::EmailOsoite) : kp()->settings()->value("EmailOsoite").toString();
    QString kopioEmail = kpasetus ? kp()->asetukset()->asetus(AsetusModel::EmailKopio) :
                                    kp()->settings()->value("EmailKopio").toString();


    SmtpClient smtp(server, port, (SmtpClient::ConnectionType) tyyppi);
    smtp.connectToHost();

    // Tehdään tässä välissä viesti valmiiksi
    const Lasku lasku = tosite_->constLasku();

    QString kenelleNimi = tosite_->kumppaninimi();
    QString kenelleEmail = lasku.email();
    QString kieli = lasku.kieli().toLower();

    LaskunTulostaja tulostaja(kp());

    QString otsikko = viestinOtsikko();


    MimeMessage message;

    message.setHeaderEncoding(MimePart::QuotedPrintable);
    message.setSender(EmailAddress(keneltaEmail, kenelta));
    if( kenelleEmail.contains(",")) {
        QStringList vastaanottajat = kenelleEmail.split(',');
        for(const auto& osoite :  vastaanottajat) {
            message.addRecipient(EmailAddress(osoite));
        }
    } else {
        message.addRecipient(EmailAddress(kenelleEmail, kenelleNimi));
    }


    if( !kopioEmail.isEmpty())
        message.addBcc(EmailAddress(kopioEmail));

    message.setSubject(otsikko);

    QString viesti = lasku.tulkkaaMuuttujat(lasku.saate());
    if(viesti.isEmpty())
        viesti = lasku.tulkkaaMuuttujat( kp()->asetukset()->asetus("EmailSaate") );

    MimeText text(viesti);
    message.addPart(&text);

    QString filename = tulkkaa("laskuotsikko",kieli).toLower() + lasku.numero() + ".pdf";
    MimeByteArrayAttachment attachment(filename, tulostaja.pdf(*tosite_));
    attachment.setContentType("application/pdf");
    message.addPart(&attachment);

    QList<MimeByteArrayAttachment*> extraAttachments;

    for(int i=0; i < tosite_->liitteet()->rowCount(); i++) {
        const QModelIndex indeksi = tosite_->liitteet()->index(i);


        if( indeksi.data(TositeLiitteet::RooliRooli).toString().isEmpty()) {

            MimeByteArrayAttachment* lisaLiite = new MimeByteArrayAttachment(indeksi.data(TositeLiitteet::NimiRooli).toString(),
                                                                    indeksi.data(TositeLiitteet::SisaltoRooli).toByteArray() );
            lisaLiite->setContentType(indeksi.data(TositeLiitteet::TyyppiRooli).toString());
            extraAttachments.append(lisaLiite);

            message.addPart(lisaLiite);

        }
    }

    // Nyt otetaan yhteys ja kirjaudutaan

    if( !smtp.waitForReadyConnected()) {
        virhe(tr("Sähköpostipalvelimeen %1 yhdistäminen epäonnistui.\nTarkista sähköpostien lähettämisen asetukset.").arg(server));
        return;
    }

    if( !password.isEmpty() ) {
        smtp.login(user, password);
        if( !smtp.waitForAuthenticated()) {
            virhe(tr("Sähköpostipalvelimelle %1 kirjautuminen epäonnistui.\nTarkista sähköpostien lähettämisen asetukset.").arg(server));
            return;
        }
    }

    smtp.sendMail(message);
    if( smtp.waitForMailSent()) {
        merkkaaToimitetuksi();
    } else {
        virhe(tr("Laskujen lähettäminen sähköpostillä epäonnistui."));
    }

    smtp.quit();

    for(auto ptr : extraAttachments) {
        delete ptr;
    }

    tosite_->deleteLater();
}

QString SahkopostiToimittaja::viestinOtsikko() const
{
    const Lasku lasku = tosite_->constLasku();
    const QString kieli = lasku.kieli().toLower();

    QString otsikko = lasku.tulkkaaMuuttujat( lasku.saateOtsikko() );
    if( otsikko.isEmpty()) {
        otsikko = QString("%3 %1 %2").arg(lasku.numero(), kp()->asetukset()->asetus(AsetusModel::OrganisaatioNimi),
            tosite_->tyyppi() == TositeTyyppi::HYVITYSLASKU ? tulkkaa("hlasku", kieli) :
                           (tosite_->tyyppi() == TositeTyyppi::MAKSUMUISTUTUS ? tulkkaa("maksumuistutus", kieli)
                                                                            : tulkkaa("laskuotsikko", kieli)));
    };
    return otsikko;
}

void SahkopostiToimittaja::lahetaKitsas()
{
    liiteIndeksi_ = -1;
    liitteet_.clear();

    LaskunTulostaja tulostaja(kp());
    QByteArray kuva = tulostaja.pdf(*tosite_);

    QString osoite = kp()->pilvi()->finvoiceOsoite() + "/attachment";
    PilviKysely *pk = new PilviKysely( kp()->pilvi(), KpKysely::POST,
                osoite );
    connect( pk, &PilviKysely::vastaus, this, &SahkopostiToimittaja::liiteLiitetty);
    pk->lahetaTiedosto(kuva);
}

void SahkopostiToimittaja::liiteLiitetty(QVariant *data)
{
    // Liitetään saapunut
    QVariantMap map = data->toMap();
    if( liiteIndeksi_ < 0) {
        map.insert("filename", tosite_->laskuNumero() + ".pdf");
        map.insert("contentType", "application/pdf");
    } else {
        const QModelIndex index = tosite_->liitteet()->index(liiteIndeksi_);
        map.insert("filename", index.data(TositeLiitteet::NimiRooli).toString());
        map.insert("contentType", index.data(TositeLiitteet::TyyppiRooli).toString());
    }
    liitteet_.append(map);

    // Siirrytään seuraavaan
    liiteIndeksi_++;

    // Hypätään laskun kuvan yli
    if( tosite_->liitteet()->index(liiteIndeksi_).data(TositeLiitteet::RooliRooli).toString() == "lasku") {
        liiteIndeksi_++;
    }

    // Jos liitteitä on jäljellä, liitetään seuraava liite, muuten siirrytään
    // eteenpäin
    if( liiteIndeksi_ < tosite_->liitteet()->rowCount()) {
        const QByteArray& sisalto = tosite_->liitteet()->index(liiteIndeksi_).data(TositeLiitteet::SisaltoRooli).toByteArray();
        QString osoite = kp()->pilvi()->finvoiceOsoite() + "/attachment";
        PilviKysely *pk = new PilviKysely( kp()->pilvi(), KpKysely::POST,
                    osoite );
        connect( pk, &PilviKysely::vastaus, this, &SahkopostiToimittaja::liiteLiitetty);
        pk->lahetaTiedosto(sisalto);

    } else {
        lahetaViesti();
    }
}

void SahkopostiToimittaja::lahetaViesti()
{
    const Lasku lasku = tosite_->constLasku();

    QVariantMap viesti;
    viesti.insert("attachments", liitteet_);
    viesti.insert("from", QString("\"%1\" %2")
                  .arg(kp()->asetukset()->asetus(AsetusModel::EmailNimi))
                  .arg(kp()->asetukset()->asetus(AsetusModel::EmailOsoite))
                  );
    if(lasku.email().contains(",")) {
        viesti.insert("to", lasku.email());
    } else {
        viesti.insert("to", QString("\"%1\" %2")
                  .arg(tosite_->kumppaninimi())
                  .arg(lasku.email())
                  );
    }
    viesti.insert("subject", viestinOtsikko());
    const QString kopioEmail = kp()->asetukset()->asetus(AsetusModel::EmailKopio);
    if( !kopioEmail.isEmpty())
        viesti.insert("bcc", kopioEmail);
    viesti.insert("text", lasku.tulkkaaMuuttujat(lasku.saate()));

    QString osoite = kp()->pilvi()->finvoiceOsoite() + "/email" ;

    PilviKysely *pk = new PilviKysely( kp()->pilvi(), KpKysely::POST,
                osoite );
    connect( pk, &PilviKysely::vastaus, this, &SahkopostiToimittaja::merkkaaToimitetuksi);
    connect( pk, &KpKysely::virhe, this, [this] {this->virhe("Laskujen lähettäminen sähköpostilla epäonnistui.");});

    pk->kysy(viesti);


    tosite_->deleteLater();

}


