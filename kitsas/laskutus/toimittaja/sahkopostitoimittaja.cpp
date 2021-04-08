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

SahkopostiToimittaja::SahkopostiToimittaja(QObject *parent)
    : AbstraktiToimittaja(parent)
{

}


void SahkopostiToimittaja::toimita()
{
    bool kpasetus = !kp()->asetukset()->asetus(AsetusModel::SmtpServer).isEmpty();
    QString server = kpasetus ? kp()->asetukset()->asetus(AsetusModel::SmtpServer) : kp()->settings()->value("SmtpServer").toString();
    int port = kpasetus ? kp()->asetukset()->luku(AsetusModel::SmtpPort) : kp()->settings()->value("SmtpPort").toInt();
    QString user = kpasetus ? kp()->asetukset()->asetus(AsetusModel::SmtpUser) : kp()->settings()->value("SmtpUser").toString();
    QString password = kpasetus ? kp()->asetukset()->asetus(AsetusModel::SmtpPassword) : kp()->settings()->value("SmtpPassword").toString();
    int tyyppi = EmailMaaritys::sslIndeksi( kpasetus ? kp()->asetukset()->asetus(AsetusModel::EmailSSL) : kp()->settings()->value("EmailSSL").toString() );
    QString kenelta = kpasetus ? kp()->asetukset()->asetus(AsetusModel::EmailNimi) : kp()->settings()->value("EmailNimi").toString();
    QString keneltaEmail = kpasetus ? kp()->asetukset()->asetus(AsetusModel::EmailOsoite) : kp()->settings()->value("EmailOsoite").toString();
    QString kopioEmail = kpasetus ? kp()->asetukset()->asetus(AsetusModel::EmailKopio) : kp()->settings()->value("EmailKopio").toString();


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

    Tosite tosite;
    tosite.lataa(tositeMap());


    QString kenelleNimi = tosite.kumppaninimi();
    QString kenelleEmail = tosite.lasku().email();
    QString kieli = tosite.lasku().kieli().toLower();

    LaskunTulostaja tulostaja(kp());

    QString otsikko = QString("%3 %1 %2").arg(tosite.lasku().numero()).arg(kp()->asetukset()->asetus(AsetusModel::OrganisaatioNimi))
            .arg( tosite.tyyppi() == TositeTyyppi::HYVITYSLASKU ? tulkkaa("hlasku", kieli) :
                           (tosite.tyyppi() == TositeTyyppi::MAKSUMUISTUTUS ? tulkkaa("maksumuistutus", kieli)
                                                                            : tulkkaa("laskuotsikko", kieli)));

    MimeMessage message;
    message.setHeaderEncoding(MimePart::QuotedPrintable);
    message.setSender(new EmailAddress(keneltaEmail, kenelta));
    message.addRecipient(new EmailAddress(kenelleEmail, kenelleNimi));
    if( !kopioEmail.isEmpty())
        message.addBcc(new EmailAddress(kopioEmail));

    message.setSubject(otsikko);

    QString viesti = tosite.lasku().saate();
    if(viesti.isEmpty())
        viesti = kp()->asetukset()->asetus("EmailSaate");

    if( kp()->asetukset()->luku("EmailMuoto")) {
        if(!viesti.isEmpty())
            viesti.append("\n\n");
        viesti.append( maksutiedot(tosite) );
    }

    MimeText text(viesti);
    message.addPart(&text);

    QString filename = tulkkaa("laskuotsikko",kieli).toLower() + tosite.lasku().numero() + ".pdf";
    MimeAttachment attachment(tulostaja.pdf(tosite), filename);
    attachment.setContentType("application/pdf");
    message.addPart(&attachment);

    if(client.sendMail(message)) {
        merkkaaToimitetuksi();
    } else {
        virhe(tr("Laskujen lähettäminen sähköpostillä epäonnistui."));
    }

}

QString SahkopostiToimittaja::maksutiedot(const Tosite &tosite)
{
    const Lasku& lasku = tosite.constLasku();
    const QString& kieli = lasku.kieli();

    QString iban = kp()->asetukset()->asetus(AsetusModel::LaskuIbanit).split(",").value(0);
    bool rf = kp()->asetukset()->onko(AsetusModel::LaskuRF);

    if( !lasku.summa().cents())
        return tulkkaa("eimaksettavaa", kieli);   // Ei maksettavaa



    QString txt = tulkkaa("erapvm", kieli) + " " + lasku.erapvm().toString("dd.MM.yyyy") + "\n";
    txt.append(tulkkaa("Yhteensa", kieli) + " " + lasku.summa().display()  + " \n");
    txt.append(tulkkaa("viitenro", kieli) + " " +  ( rf ? lasku.viite().rfviite() : lasku.viite().valeilla()  )  + "\n");
    txt.append(tulkkaa("iban", kieli) + " " + Iban(iban).valeilla() + "\n");
    txt.append(tulkkaa("virtviiv", kieli) + " " + lasku.virtuaaliviivakoodi(iban, rf)  );

    return txt;

}
