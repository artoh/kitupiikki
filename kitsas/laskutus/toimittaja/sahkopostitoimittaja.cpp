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

#include "liite/liitteetmodel.h"
#include "pilvi/pilvikysely.h"


SahkopostiToimittaja::SahkopostiToimittaja(QObject *parent)
    : AbstraktiToimittaja(parent)
{

}


void SahkopostiToimittaja::toimita()
{
    ema_.lataa();
    if( ema_.kitsasPalvelin()) {
        lahetaKitsas();
    } else {
        lahetaSmtp();
    }

}

QString SahkopostiToimittaja::viestinOtsikko()
{
    const Lasku lasku = tosite()->constLasku();
    const QString kieli = lasku.kieli().toLower();

    QString otsikko = lasku.tulkkaaMuuttujat( lasku.saateOtsikko() );
    if( otsikko.isEmpty()) {
        otsikko = QString("%3 %1 %2").arg(lasku.numero(), lasku.toiminimiTieto(ToiminimiModel::Nimi),
            tosite()->tyyppi() == TositeTyyppi::HYVITYSLASKU ? tulkkaa("hlasku", kieli) :
                           (tosite()->tyyppi() == TositeTyyppi::MAKSUMUISTUTUS ? tulkkaa("maksumuistutus", kieli)
                                                                            : tulkkaa("laskuotsikko", kieli)));
    };
    return otsikko;
}

void SahkopostiToimittaja::lahetaSmtp()
{
    SmtpClient smtp( ema_.palvelin(),  ema_.portti(), (SmtpClient::ConnectionType) ema_.suojaus());
    smtp.connectToHost();

    // Tehdään tässä välissä viesti valmiiksi
    const Lasku lasku = tosite()->constLasku();

    QString kenelleNimi = tosite()->kumppaninimi();
    QString kenelleEmail = lasku.email();
    QString kieli = lasku.kieli().toLower();

    LaskunTulostaja tulostaja(kp());

    QString otsikko = viestinOtsikko();


    MimeMessage message;

    message.setHeaderEncoding(MimePart::QuotedPrintable);
    message.setSender(EmailAddress(ema_.lahettajaOsoite(), ema_.lahettajaNimi()));
    if( kenelleEmail.contains(",")) {
        QStringList vastaanottajat = kenelleEmail.split(',');
        for(const auto& osoite :  vastaanottajat) {
            message.addRecipient(EmailAddress(osoite));
        }
    } else {
        message.addRecipient(EmailAddress(kenelleEmail, kenelleNimi));
    }


    if( !ema_.kopioOsoite().isEmpty())
        message.addBcc(EmailAddress(ema_.kopioOsoite()));

    message.setSubject(otsikko);

    QString viesti = lasku.tulkkaaMuuttujat(lasku.saate());
    if(viesti.isEmpty())
        viesti = lasku.tulkkaaMuuttujat( kp()->asetukset()->asetus("EmailSaate") );

    MimeText text(viesti);
    message.addPart(&text);

    QString filename = tulkkaa("laskuotsikko",kieli).toLower() + lasku.numero() + ".pdf";
    MimeByteArrayAttachment attachment(filename, tulostaja.pdf(*tosite()));
    attachment.setContentType("application/pdf");
    message.addPart(&attachment);

    QList<MimeByteArrayAttachment*> extraAttachments;

    for(int i=0; i < tosite()->liitteet()->rowCount(); i++) {
        const QModelIndex indeksi = tosite()->liitteet()->index(i);


        if( indeksi.data(LiitteetModel::RooliRooli).toString().isEmpty()) {

            MimeByteArrayAttachment* lisaLiite = new MimeByteArrayAttachment(indeksi.data(LiitteetModel::NimiRooli).toString(),
                                                                    indeksi.data(LiitteetModel::SisaltoRooli).toByteArray() );
            lisaLiite->setContentType(indeksi.data(LiitteetModel::TyyppiRooli).toString());
            extraAttachments.append(lisaLiite);

            message.addPart(lisaLiite);

        }
    }

    // Nyt otetaan yhteys ja kirjaudutaan

    if( !smtp.waitForReadyConnected()) {
        virhe(tr("Sähköpostipalvelimeen %1 yhdistäminen epäonnistui.\nTarkista sähköpostien lähettämisen asetukset.").arg( ema_.palvelin()));
        return;
    }

    if( !ema_.kayttaja().isEmpty() || !ema_.salasana().isEmpty()) {
        smtp.login(ema_.kayttaja(), ema_.salasana());
        if( !smtp.waitForAuthenticated()) {
            virhe(tr("Sähköpostipalvelimelle %1 kirjautuminen epäonnistui.\nTarkista sähköpostien lähettämisen asetukset.").arg(ema_.palvelin()));
            return;
        }
    }

    smtp.sendMail(message);
    if( smtp.waitForMailSent()) {
        valmis();
    } else {
        virhe(tr("Laskujen lähettäminen sähköpostilla epäonnistui."));
    }

    smtp.quit();

    for(auto ptr : extraAttachments) {
        delete ptr;
    }
}

void SahkopostiToimittaja::lahetaKitsas()
{

    liitteet_.clear();

    LaskunTulostaja tulostaja(kp());
    QByteArray kuva = tulostaja.pdf(*tosite()).toBase64();

    QVariantMap map;
    map.insert("content", kuva);
    map.insert("filename", tulkkaa("laskuotsikko",tosite()->constLasku().kieli() ).toLower() + tosite()->laskuNumero() + ".pdf");
    map.insert("contentType", "application/pdf");
    liitteet_.append(map);

    for (int liiteIndeksi = 0; liiteIndeksi < tosite()->liitteet()->rowCount(); liiteIndeksi++) 
   {
        // Hypätään laskun kuvan yli
        if( tosite()->liitteet()->index(liiteIndeksi).data(LiitteetModel::RooliRooli).toString() == "lasku") {
            continue;
        }

        QVariantMap map;

        const QModelIndex index = tosite()->liitteet()->index(liiteIndeksi);
        map.insert("content", index.data(LiitteetModel::SisaltoRooli).toByteArray().toBase64());
        map.insert("filename", index.data(LiitteetModel::NimiRooli).toString());
        map.insert("contentType", index.data(LiitteetModel::TyyppiRooli).toString());

        liitteet_.append(map);
    }

    lahetaViesti();

}

void SahkopostiToimittaja::lahetaViesti()
{
    const Lasku lasku = tosite()->constLasku();

    QVariantMap viesti;
    viesti.insert("attachments", liitteet_);
    viesti.insert("senderName", ema_.lahettajaNimi());
    viesti.insert("senderAddress", ema_.lahettajaOsoite());
    viesti.insert("docid", tosite()->id());

    if(lasku.email().contains(",")) {
        viesti.insert("to", lasku.email());
    } else {
        viesti.insert("to", QString("\"%1\" %2")
                  .arg(tosite()->kumppaninimi(),lasku.email()) );
    }
    viesti.insert("subject", viestinOtsikko());
    const QString kopioEmail = kp()->asetukset()->asetus(AsetusModel::EmailKopio);
    if( !kopioEmail.isEmpty())
        viesti.insert("bcc", kopioEmail);
    viesti.insert("text", lasku.tulkkaaMuuttujat(lasku.saate()));

    QString osoite = kp()->pilvi()->finvoiceOsoite() + "/email" ;

    PilviKysely *pk = new PilviKysely( kp()->pilvi(), KpKysely::POST,
                osoite );
    connect( pk, &PilviKysely::vastaus, this, [this] {this->valmis();});
    connect( pk, &KpKysely::virhe, this, [this] {this->virhe("Laskujen lähettäminen sähköpostilla epäonnistui.");});

    pk->kysy(viesti);


}


