/*
   Copyright (C) 2018 Arto Hyvättinen

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
#include "finvoice.h"

#include <QXmlStreamWriter>
#include <QTextStream>
#include "db/kirjanpito.h"

#include "laskuntulostaja.h"
#include "validator/ibanvalidator.h"

#include <QRegularExpression>
#include <QRegularExpressionMatch>
#include <QFile>
#include <QDir>

#include <zip.h>

Finvoice::Finvoice(QObject *parent) : QObject(parent)
{

}

bool Finvoice::muodostaFinvoice(LaskuModel *model)
{
    QDir hakemisto( kp()->asetukset()->asetus("VerkkolaskuKansio") );

    if( !kp()->asetukset()->onko("VerkkolaskuZip"))
    {
        QFile xmlTiedosto( hakemisto.absoluteFilePath(QString("lasku-%1.xml").arg( model->laskunro() )) );
        if( !xmlTiedosto.open( QIODevice::WriteOnly ))
            return false;
        xmlTiedosto.write( lasku(model) );
        xmlTiedosto.close();

        if( kp()->asetukset()->onko("VerkkolaskuPdf"))
        {
            QFile pdfTiedosto( hakemisto.absoluteFilePath(QString("lasku-%1.pdf").arg(model->laskunro())) );
            if( !pdfTiedosto.open( QIODevice::WriteOnly))
                return false;
            LaskunTulostaja tulostaja(model);
            pdfTiedosto.write( tulostaja.pdf() );
        }
        return true;
    }
    // Tässä tehdään zip

    int virhekoodi = 0;
    zip_t* paketti = zip_open( hakemisto.absoluteFilePath(QString("lasku-%1.zip").arg(model->laskunro())).toStdString().c_str(),
                               ZIP_CREATE | ZIP_TRUNCATE, &virhekoodi);
    if( !paketti)
        return false;

    QByteArray ba = lasku(model);

    zip_error_t virhe;
    zip_source_t* puskuri = zip_source_buffer_create( ba.data(), static_cast<zip_uint16_t>( ba.length() ), 0,  &virhe);
    if( !puskuri)
        return false;
    if( zip_file_add(paketti, QString("lasku-%1.xml").arg(model->laskunro()).toStdString().c_str(),
                     puskuri, 0) < 0)
        return false;


    LaskunTulostaja tulostaja(model);
    QByteArray pdfba = tulostaja.pdf();

    zip_source_t* pdfPuskuri = zip_source_buffer_create( pdfba.data(), static_cast<zip_uint16_t>( pdfba.length() ), 0,  &virhe);
    if( !pdfPuskuri)
        return false;
    if( zip_file_add(paketti, QString("lasku-%1.pdf").arg(model->laskunro()).toStdString().c_str(),
                     pdfPuskuri, 0) < 0)
        return false;

    zip_close(paketti);

    return true;
}

QByteArray Finvoice::lasku(LaskuModel *model)
{
    QByteArray soapArray;

    QString lahettajanVerkkolasku = kp()->asetukset()->asetus("VerkkolaskuOsoite");
    QString lahettajanValittaja = kp()->asetukset()->asetus("VerkkolaskuValittaja");
    QString vastaanottajanVerkkolasku = model->verkkolaskuOsoite();
    QString vastaanottajanValittaja =  model->verkkolaskuValittaja();
    QString aikaleima = QDateTime::currentDateTime().toString("yyyy-MM-ddThh:mm:ss");
    QString alvtunnus = QString("FI%1").arg(kp()->asetukset()->asetus("Ytunnus"));
    alvtunnus.remove('-');
    QString iban = kp()->tilit()->tiliNumerollaVanha( kp()->asetukset()->luku("LaskuTili")).json()->str("IBAN");

    if( kp()->asetukset()->onko("VerkkolaskuSOAP") )  // Kirjoita SOAP
    {
        QTextStream out(&soapArray);
        out << R"(<SOAP-ENV:Envelope xmlns:SOAP-ENV="http://schemas.xmlsoap.org/soap/envelope/" xmlns:xlink="http://www.w3.org/1999/xlink" xmlns:eb="http://www.oasis-open.org/committees/ebxml-msg/schema/msg-header-2_0.xsd">)" << "\n";
        out << R"(<SOAP-ENV:Header>)" << "\n";
        out << R"(<eb:MessageHeader xmlns:eb="http://www.oasis-open.org/committees/ebxml-msg/schema/msg-header-2_0.xsd" SOAP-ENV:mustUnderstand="1" eb:version="2.0">)" << "\n";
        out << R"(<eb:From>)" << "\n";
        out << R"(<eb:PartyId>)" << lahettajanVerkkolasku << R"(</eb:PartyId>)" << "\n";
        out << R"(<eb:Role>Sender</eb:Role>)" << "\n";
        out << R"(</eb:From>)" << "\n";
        out << R"(<eb:From>)" << "\n";
        out << R"(<eb:PartyId>)" << lahettajanValittaja << R"(</eb:PartyId>)" << "\n";
        out << R"(<eb:Role>Intermediator</eb:Role>)" << "\n";
        out << R"(</eb:From>)" << "\n";
        out << R"(<eb:To>)" << "\n";
        out << R"(<eb:PartyId>)" << vastaanottajanVerkkolasku << R"(</eb:PartyId>)" << "\n";
        out << R"(<eb:Role>Sender</eb:Role>)" << "\n";
        out << R"(</eb:To>)" << "\n";
        out << R"(<eb:To>)" << "\n";
        out << R"(<eb:PartyId>)" << vastaanottajanValittaja << R"(</eb:PartyId>)" << "\n";
        out << R"(<eb:Role>Intermediator</eb:Role>)" << "\n";
        out << R"(</eb:To>)" << "\n";
        out << R"(<eb:CPAId>yoursandmycpa</eb:CPAId>)" << "\n";
        out << R"(<eb:ConversationId></eb:ConversationId>)" << "\n";
        out << R"(<eb:Service>Routing</eb:Service>)" << "\n";
        out << R"(<eb:Action>ProcessInvoice</eb:Action>)" << "\n";
        out << R"(<eb:MessageData>)" << "\n";
        out << R"(<eb:MessageId>)" << model->laskunro() <<  R"(</eb:MessageId>)" << "\n";
        out << R"(<eb:Timestamp>)" << aikaleima << R"(</eb:Timestamp>)" << "\n";
        out << R"(</eb:MessageData>)" << "\n";
        out << R"(</eb:MessageHeader>)" << "\n";
        out << R"(</SOAP-ENV:Header>)" << "\n";
        out << R"(<SOAP-ENV:Body>)" << "\n";
        out << R"(<eb:Manifest eb:id="Manifest" eb:version="2.0">)" << "\n";
        out << R"(<eb:Reference eb:id="Finvoice" xlink:href="200911180001">)" << "\n";
        out << R"(<eb:Schema eb:location="http://www.finvoice.info/finvoice.xsd" eb:version="2.0"/>)" << "\n";
        out << R"(</eb:Reference>)" << "\n";
        out << R"(</eb:Manifest>)" << "\n";
        out << R"(</SOAP-ENV:Body>)" << "\n";
        out << R"(</SOAP-ENV:Envelope>)" << "\n";


    }
    QByteArray byteArray;

    QXmlStreamWriter writer(&byteArray);
    writer.setAutoFormatting(true);
    writer.setAutoFormattingIndent(4);
    writer.setCodec("ISO-8859-15");

    writer.writeStartDocument("1.0");
    writer.writeStartElement("Finvoice");
    writer.writeAttribute("Version","2.01");

    writer.writeStartElement("MessageTransmissionDetails");

    writer.writeStartElement("MessageSenderDetails");
    writer.writeTextElement("FromIdentifier", lahettajanVerkkolasku);
    writer.writeTextElement("FromIntermediator", lahettajanValittaja);
    writer.writeEndElement();

    writer.writeStartElement("MessageReceiverDetails");
    writer.writeTextElement("ToIdentifier", vastaanottajanVerkkolasku);
    writer.writeTextElement("ToIntermediator", vastaanottajanValittaja);
    writer.writeEndElement();

    writer.writeStartElement("MessageDetails");
    writer.writeTextElement("MessageIdentifier", QString::number( model->laskunro() ));
    writer.writeTextElement("MessageTimeStamp", aikaleima);
    writer.writeEndElement();

    writer.writeEndElement();

    writer.writeStartElement("SellerPartyDetails");
    writer.writeTextElement("SellerPartyIdentifier", kp()->asetukset()->asetus("Ytunnus"));
    writer.writeTextElement("SellerOrganisationName", kp()->asetukset()->asetus("Nimi"));

    if( kp()->asetukset()->onko("AlvVelvollinen"))
        writer.writeTextElement("SellerOrganisationTaxCode", alvtunnus );

    HajoitettuOsoite myyjanOsoite = hajoitaOsoite( kp()->asetukset()->asetus("Osoite") );
    writer.writeStartElement("SellerPostalAddressDetails");
    writer.writeTextElement("SellerStreetName", myyjanOsoite.lahiosoite);
    writer.writeTextElement("SellerTownName", myyjanOsoite.postitoimipaikka);
    writer.writeTextElement("SellerPostCodeIdentifier", myyjanOsoite.postinumero);
    writer.writeEndElement();    

    writer.writeEndElement();

    writer.writeStartElement("SellerInformationDetails");

    if( kp()->asetukset()->onko("Puhelin"))
        writer.writeTextElement("SellerPhoneNumber", kp()->asetukset()->asetus("Puhelin"));

    if( kp()->asetukset()->onko("Sahkoposti"))
        writer.writeTextElement("SellerCommonEmailaddressIdentifier", kp()->asetukset()->asetus("Sahkoposti"));

    writer.writeStartElement("SellerAccountDetails");

    writer.writeStartElement("SellerAccountID");
    writer.writeAttribute("IdentificationsSchemeName","IBAN");
    writer.writeCharacters(iban );
    writer.writeEndElement();
    writer.writeStartElement("SellerBic");
    writer.writeAttribute("IdentificationsSchemeName", "BIC");
    writer.writeCharacters( LaskutModel::bicIbanilla(iban) );
    writer.writeEndElement();

    writer.writeEndElement();
    writer.writeEndElement();

    writer.writeStartElement("BuyerPartyDetails");
    if( !model->ytunnus().isEmpty() && model->ytunnus().at(0).isDigit())
        writer.writeTextElement("ByerPartyIdentifier", model->ytunnus());
    writer.writeTextElement("BuyerOrganisationName", model->laskunsaajanNimi());

    if( !model->ytunnus().isEmpty() && model->ytunnus().at(0).isLetter())
        writer.writeTextElement("BuyerOrganisationTaxCode", model->ytunnus());

    writer.writeStartElement("BuyerPostalAddressDetails");
    HajoitettuOsoite asiakkaanOsoite = hajoitaOsoite( model->osoite());
    writer.writeTextElement("BuyerStreetName", asiakkaanOsoite.lahiosoite);
    writer.writeTextElement("BuyerTownName", asiakkaanOsoite.postitoimipaikka);
    writer.writeTextElement("BuyerPostCodeIdentifier", asiakkaanOsoite.postinumero);
    writer.writeTextElement("CountryCode", asiakkaanOsoite.maakoodi);
    writer.writeEndElement();
    writer.writeEndElement();

    writer.writeStartElement("DeliveryDetails");
    writer.writeStartElement("DeliveryDate");
    writer.writeAttribute("Format","CCYYMMDD");
    writer.writeCharacters( model->toimituspaiva().toString("yyyyMMdd") );
    writer.writeEndElement();
    writer.writeEndElement();

    writer.writeStartElement("InvoiceDetails");
    writer.writeTextElement("InvoiceTypeCode","INV01");
    writer.writeTextElement("InvoiceTypeText","LASKU");
    writer.writeTextElement("OriginCode","Original");
    writer.writeTextElement("InvoiceNumber", QString::number(model->laskunro()));
    writer.writeStartElement("InvoiceDate");
    if( !model->asiakkaanViite().isEmpty())
        writer.writeTextElement("BuyerReferenceIdentifier", model->asiakkaanViite());
    writer.writeAttribute("Format","CCYYMMDD");
    writer.writeCharacters( QDate::currentDate().toString("yyyyMMdd") );
    writer.writeEndElement();

    // Hakee alv-erittelyt
    QList<AlvErittelyRivi> alvErittely = model->alverittely();

    writer.writeStartElement("InvoiceTotalVatExcludedAmount");
    writer.writeAttribute("AmountCurrencyIdentifier","EUR");
    writer.writeCharacters( QString("%1").arg( model->nettoSumma() / 100.0 , 0, 'f', 2).replace('.',',') );
    writer.writeEndElement();

    writer.writeStartElement("InvoiceTotalVatAmount");
    writer.writeAttribute("AmountCurrencyIdentifier","EUR");
    writer.writeCharacters( QString("%1").arg( ( model->laskunSumma() - model->nettoSumma() ) / 100.0 , 0, 'f', 2).replace('.',',') );
    writer.writeEndElement();

    writer.writeStartElement("InvoiceTotalVatIncludedAmount");
    writer.writeAttribute("AmountCurrencyIdentifier","EUR");
    writer.writeCharacters( QString("%1").arg( model->laskunSumma() / 100.0, 0, 'f', 2 ).replace('.',',') );
    writer.writeEndElement();

    for( AlvErittelyRivi alv : alvErittely)
    {

        writer.writeStartElement("VatSpecificationDetails");
        writer.writeStartElement("VatBaseAmount");
        writer.writeAttribute("AmountCurrencyIdentifier","EUR");
        writer.writeCharacters( QString("%1").arg( alv.netto() / 100.0 , 0, 'f', 2).replace('.',',') );
        writer.writeEndElement();

        writer.writeTextElement("VatCode", vatCode(alv.alvKoodi()) );

        if( alv.vero() > 1e-5)
            writer.writeTextElement("VatRatePercent", QString("%1,0").arg( alv.alvProsentti() ) );

        writer.writeStartElement("VatRateAmount");
        writer.writeAttribute("AmountCurrencyIdentifier","EUR");
        writer.writeCharacters( QString("%1").arg( alv.vero() / 100.0 , 0, 'f', 2).replace('.',',') );
        writer.writeEndElement();

        QString vatTeksti = vatFree( alv.alvKoodi() );
        if( !vatTeksti.isEmpty())
            writer.writeTextElement("VatFreeText", vatTeksti);

        writer.writeEndElement();
    }

    if( !model->lisatieto().isEmpty())
        writer.writeTextElement("InvoiceFreeText", model->lisatieto());

    writer.writeStartElement("PaymentTermsDetails");
    writer.writeStartElement("InvoiceDueDate");
    writer.writeAttribute("Format","CCYYMMDD");
    writer.writeCharacters( model->erapaiva().toString("yyyyMMdd") );
    writer.writeEndElement();
    if( model->viivastysKorko() > 1e-5)
    {
        writer.writeStartElement("PaymentOverDueFineDetails");
        writer.writeTextElement("PaymentOverDueFineFreeText", "Viivästyskorko");
        writer.writeTextElement("PaymentOverDueFinePercent", QString::number(model->viivastysKorko(),'g',1) );
        writer.writeEndElement();
    }
    writer.writeEndElement();
    writer.writeEndElement();   // InvoiceDetails

    // Laskun rivit
    for(int i=0; i < model->rowCount(QModelIndex()); i++)
    {
        QModelIndex indeksi = model->index(i,0);
        if( !indeksi.data(LaskuModel::NettoRooli).toLongLong() )
            continue;

        writer.writeStartElement("InvoiceRow");

        writer.writeTextElement("ArticleName", indeksi.data(LaskuModel::NimikeRooli).toString() );

        writer.writeStartElement("OrderedQuantity");
        writer.writeAttribute("QuantityUnitCode", indeksi.sibling(i, LaskuModel::YKSIKKO).data().toString());
        writer.writeCharacters( indeksi.sibling(i, LaskuModel::MAARA).data().toString() );
        writer.writeEndElement();

        writer.writeStartElement("InvoicedQuantity");
        writer.writeAttribute("QuantityUnitCode", indeksi.sibling(i, LaskuModel::YKSIKKO).data().toString());
        writer.writeCharacters( indeksi.sibling(i, LaskuModel::MAARA).data().toString() );
        writer.writeEndElement();

        writer.writeStartElement("UnitPriceAmount");
        writer.writeAttribute("AmountCurrencyIdentifier","EUR");
        writer.writeCharacters( QString("%1").arg( indeksi.data(LaskuModel::AHintaRooli).toLongLong() / 100.0 , 0, 'f', 2).replace('.',','));
        writer.writeEndElement();

        if( indeksi.data(LaskuModel::AleProsenttiRooli).toInt())
        {
            writer.writeStartElement("RowDiscountPercent");
            writer.writeCharacters( QString("%1,0").arg( indeksi.data(LaskuModel::AleProsenttiRooli).toInt() ));
            writer.writeEndElement();
        }

        double verosnt = model->data( model->index(i, LaskuModel::NIMIKE), LaskuModel::VeroRooli ).toDouble();

        writer.writeTextElement("RowPositionIdentifier", QString::number( i + 1));

        writer.writeTextElement("RowVatRatePercent",  QString("%1,0").arg(indeksi.data(LaskuModel::AlvProsenttiRooli).toInt()));

        int alvkoodi = indeksi.data(LaskuModel::AlvKoodiRooli).toInt();
        writer.writeTextElement("RowVatCode", vatCode( alvkoodi ) );

        writer.writeStartElement("RowVatAmount");
        writer.writeAttribute("AmountCurrencyIdentifier","EUR");
        writer.writeCharacters( QString("%1").arg( verosnt / 100.0 , 0, 'f', 2).replace('.',',') );
        writer.writeEndElement();

        writer.writeStartElement("RowVatExcludedAmount");
        writer.writeAttribute("AmountCurrencyIdentifier","EUR");
        writer.writeCharacters( QString("%1").arg( indeksi.data(LaskuModel::BruttoRooli).toLongLong() / 100.0 , 0, 'f', 2).replace('.',','));
        writer.writeEndElement();


        writer.writeEndElement();
    }

    // EPI
    writer.writeStartElement("EpiDetails");
    writer.writeStartElement("EpiIdentificationDetails");

    writer.writeStartElement("EpiDate");
    writer.writeAttribute("Format","CCYYMMDD");
    writer.writeCharacters( QDate::currentDate().toString("yyyyMMdd") );
    writer.writeEndElement();
    writer.writeTextElement("EpiReference","0");
    writer.writeEndElement();

    writer.writeStartElement("EpiPartyDetails");
    writer.writeStartElement("EpiBfiPartyDetails");
    writer.writeStartElement("EpiBfiIdentifier");
    writer.writeAttribute("IdentificationSchemeName", "BIC");
    writer.writeCharacters( LaskutModel::bicIbanilla(iban) );
    writer.writeEndElement();
    writer.writeEndElement();

    writer.writeStartElement("EpiBeneficiaryPartyDetails");
    writer.writeTextElement("EpiNameAddressDetails", kp()->asetukset()->asetus("Nimi"));
    writer.writeTextElement("EpiBei", alvtunnus.mid(2));
    writer.writeStartElement("EpiAccountID");
    writer.writeAttribute("IdentificationSchemeName","IBAN");
    writer.writeCharacters(iban );
    writer.writeEndElement();   // EpiAccountID
    writer.writeEndElement();   // EpiBeneficiaryPartyDetails
    writer.writeEndElement();   // EpiPartyDetails

    writer.writeStartElement("EpiPaymentInstructionDetails");
    writer.writeStartElement("EpiRemittanceInfoIdentifier");

    if( kp()->asetukset()->onko("LaskuRF"))
    {
        // RF-muotoinen viite
        QString rf= "RF00" + model->viitenumero();
        int tarkiste = 98 - IbanValidator::ibanModulo( rf );
        writer.writeAttribute("IdentificationSchemeName", "ISO");
        writer.writeCharacters( QString("RF%1%2").arg(tarkiste,2,10,QChar('0')).arg(rf.mid(4)) );

    }
    else
    {
        writer.writeAttribute("IdentificationsSchemeName", "SPY");
        writer.writeCharacters( model->viitenumero() );

    }
    writer.writeEndElement();   // EpiRemittanceInfoIdentifier

    writer.writeStartElement("EpiInstructedAmount");
    writer.writeAttribute("AmountCurrencyIdentifier","EUR");
    writer.writeCharacters( QString("%1").arg( model->laskunSumma() / 100.0 , 0, 'f', 2).replace('.',',')  );
    writer.writeEndElement();

    writer.writeStartElement("EpiCharge");
    writer.writeAttribute("ChargeOption","SLEV");    
    writer.writeEndElement();

    writer.writeStartElement("EpiDateOptionDate");
    writer.writeAttribute("Format","CCYYMMDD");
    writer.writeCharacters( model->erapaiva().toString("yyyyMMdd") );
    writer.writeEndElement();

    writer.writeEndElement();   // EpiPaymentInstructionDetails
    writer.writeEndElement();   // EpiDetails

    writer.writeEndDocument();

    soapArray.append(byteArray);
    return soapArray;
}

HajoitettuOsoite Finvoice::hajoitaOsoite(const QString &osoite)
{
    QRegularExpression osoiteRe(R"((.*\n)*(?<lahi>.+)\n(?<maa>[A-Z]{1,4})?(?<numero>[0-9]{5})\s(?<paikka>.+)(\n.*)?)");
    HajoitettuOsoite hajalla;
    QRegularExpressionMatch mats = osoiteRe.match(osoite);
    if( mats.hasMatch())
    {
        hajalla.lahiosoite = mats.captured("lahi");
        hajalla.postinumero = mats.captured("numero");
        hajalla.postitoimipaikka = mats.captured("paikka");
        hajalla.maakoodi = mats.captured("maa").isEmpty() ? "FI" : mats.captured("maa");
    }
    return hajalla;
}

QString Finvoice::vatCode(int koodi)
{
    switch (koodi)
    {
    case AlvKoodi::MYYNNIT_MARGINAALI:
        return "AB";
    case AlvKoodi::RAKENNUSPALVELU_MYYNTI:
        return "AE";
    case AlvKoodi::YHTEISOMYYNTI_TAVARAT:
    case AlvKoodi::YHTEISOMYYNTI_PALVELUT:
        return "E";
    case AlvKoodi::ALV0:
        return "Z";
    default:
        return "S";
    }
}

QString Finvoice::vatFree(int koodi)
{
    switch (koodi) {
    case AlvKoodi::MYYNNIT_MARGINAALI:
        return "Marginaalivero";
    case AlvKoodi::RAKENNUSPALVELU_MYYNTI:
        return "Käännetty ALV";
    case AlvKoodi::YHTEISOMYYNTI_PALVELUT:
    case AlvKoodi::YHTEISOHANKINNAT_TAVARAT:
        return "Yhteisömyynti";
    case AlvKoodi::ALV0:
        return "Veroton myynti";
    default:
        return QString();
    }
}
