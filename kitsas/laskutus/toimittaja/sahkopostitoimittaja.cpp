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


SahkopostiToimittaja::SahkopostiToimittaja(QObject *parent)
    : AbstraktiToimittaja(parent)
{

}


void SahkopostiToimittaja::toimita()
{
    Tosite* tosite = new Tosite(this);    
    tosite->lataa(tositeMap());

    connect( tosite->liitteet(), &TositeLiitteet::kaikkiLiitteetHaettu, [this, tosite] { this->laheta(tosite); });
    tosite->liitteet()->lataaKaikkiLiitteet();
}

void SahkopostiToimittaja::laheta(Tosite *tosite)
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


    SmtpClient client(server, port, (SmtpClient::ConnectionType) tyyppi);
    if( !password.isEmpty()) {
        client.setUser(user);
        client.setPassword(password);
    }

    if( !client.connectToHost()) {
        virhe(tr("Sähköpostipalvelimeen %1 yhdistäminen epäonnistui.\nTarkista sähköpostien lähettämisen asetukset.").arg(server));
        return;
    } else if( !password.isEmpty() && !client.login() ) {
        virhe(tr("Sähköpostipalvelimelle %1 kirjautuminen epäonnistui.\nTarkista sähköpostien lähettämisen asetukset.").arg(server));
        return;
    }

    QString kenelleNimi = tosite->kumppaninimi();
    QString kenelleEmail = tosite->lasku().email();
    QString kieli = tosite->lasku().kieli().toLower();

    LaskunTulostaja tulostaja(kp());

    QString otsikko = tosite->lasku().tulkkaaMuuttujat( tosite->lasku().saateOtsikko() );
    if( otsikko.isEmpty()) {
        otsikko = QString("%3 %1 %2").arg(tosite->lasku().numero(), kp()->asetukset()->asetus(AsetusModel::OrganisaatioNimi),
            tosite->tyyppi() == TositeTyyppi::HYVITYSLASKU ? tulkkaa("hlasku", kieli) :
                           (tosite->tyyppi() == TositeTyyppi::MAKSUMUISTUTUS ? tulkkaa("maksumuistutus", kieli)
                                                                            : tulkkaa("laskuotsikko", kieli)));
    };

    MimeMessage message;
    message.setHeaderEncoding(MimePart::QuotedPrintable);
    message.setSender(new EmailAddress(keneltaEmail, kenelta));
    message.addRecipient(new EmailAddress(kenelleEmail, kenelleNimi));

    if( !kopioEmail.isEmpty())
        message.addBcc(new EmailAddress(kopioEmail));

    message.setSubject(otsikko);

    QString viesti = tosite->lasku().tulkkaaMuuttujat(tosite->lasku().saate());
    if(viesti.isEmpty())
        viesti = tosite->lasku().tulkkaaMuuttujat( kp()->asetukset()->asetus("EmailSaate") );

    MimeText text(viesti);
    message.addPart(&text);

    QString filename = tulkkaa("laskuotsikko",kieli).toLower() + tosite->lasku().numero() + ".pdf";
    MimeAttachment attachment(tulostaja.pdf(*tosite), filename);
    attachment.setContentType("application/pdf");
    message.addPart(&attachment);

    for(int i=0; i < tosite->liitteet()->rowCount(); i++) {
        const QModelIndex indeksi = tosite->liitteet()->index(i);


        if( indeksi.data(TositeLiitteet::RooliRooli).toString().isEmpty()) {

            MimeAttachment* lisaLiite = new MimeAttachment(indeksi.data(TositeLiitteet::SisaltoRooli).toByteArray(),
                                     indeksi.data(TositeLiitteet::NimiRooli).toString());
            lisaLiite->setContentType(indeksi.data(TositeLiitteet::TyyppiRooli).toString());
            lisaLiite->setParent(tosite);

            message.addPart(lisaLiite);

        }
    }

    if(client.sendMail(message)) {
        merkkaaToimitetuksi();
    } else {
        virhe(tr("Laskujen lähettäminen sähköpostillä epäonnistui."));
    }

    tosite->deleteLater();
}

