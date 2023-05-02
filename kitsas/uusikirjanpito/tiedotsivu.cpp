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
#include "tiedotsivu.h"
#include "kieli/monikielinen.h"
#include "ui_uusitiedot.h"
#include "validator/ibanvalidator.h"
#include "validator/ytunnusvalidator.h"
#include "rekisteri/postinumerot.h"
#include "pilvi/pilvimodel.h"

#include "db/kirjanpito.h"
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QJsonDocument>
#include <QMessageBox>

#include "rekisteri/postinumerot.h"

TiedotSivu::TiedotSivu(UusiVelho *wizard) :
    ui( new Ui::UusiTiedot),
    velho( wizard )
{
    ui->setupUi(this);

    setTitle(tr("Organisaation tiedot"));
    ui->tiliLine->setValidator( new IbanValidator );
    ui->ytunnusEdit->setValidator( new YTunnusValidator);

    registerField("nimi*", ui->nimiEdit);
    registerField("tili", ui->tiliLine);
    registerField("ytunnus", ui->ytunnusEdit);

    connect(ui->postinumeroEdit, &QLineEdit::textEdited, this, [this] (const QString& numero) { this->ui->kaupunkiEdit->setText( Postinumerot::toimipaikka(numero)); });

}

void TiedotSivu::initializePage()
{
    // Haetaan muodot
    ui->muotoList->clear();
    QVariantMap muotoMap = velho->asetukset_.value("muodot").toMap();
    QMapIterator<QString,QVariant> muotoIter(muotoMap);
    while( muotoIter.hasNext()) {
        muotoIter.next();
        Monikielinen kk( muotoIter.value() );
        QListWidgetItem *item = new QListWidgetItem(kk.teksti(), ui->muotoList);
        item->setData(Qt::UserRole, muotoIter.key());
    }
    ui->muotoList->setCurrentRow(0);

    // Haetaan laajuudet
    ui->laajuusList->clear();
    QVariantMap laajuusMap = velho->asetukset_.value("laajuudet").toMap();
    QMapIterator<QString,QVariant> laajuusIter(laajuusMap);
    while( laajuusIter.hasNext()) {
        laajuusIter.next();
        Monikielinen kk( laajuusIter.value() );
        QListWidgetItem *item = new QListWidgetItem(kk.teksti(), ui->laajuusList);
        item->setData(Qt::UserRole, laajuusIter.key());
    }

    if( field("ytunnus").toString().isEmpty() ) {
        ui->laajuusList->setCurrentRow( velho->asetukset_.value("laajuus").toInt() - 1);
    } else {
        // Kitsas PRO oletuksena kaikki tilit näkyvissä
        ui->laajuusList->setCurrentRow( ui->laajuusList->count() - 1 );
    }

    connect( ui->ytunnusEdit , &QLineEdit::textEdited, this, &TiedotSivu::haeytunnarilla);

    if( !ui->ytunnusEdit->text().isEmpty())
        haeytunnarilla();
}

bool TiedotSivu::validatePage()
{
    if( !ui->ytunnusEdit->text().isEmpty() && !YTunnusValidator::kelpaako(ui->ytunnusEdit->text())) {
        QMessageBox::critical(this, tr("Perustiedot"), tr("Y-tunnuksen muoto virheellinen"));
        return false;
    }
    if( !ui->tiliLine->text().isEmpty() && !IbanValidator::kelpaako(ui->tiliLine->text())) {
        QMessageBox::critical(this, tr("Perustiedot"), tr("Tilinumeron muoto virheellinen. Tilinumero on syötettävä IBAN-muodossa"));
        return false;
    }

    velho->asetukset_.insert("Nimi", ui->nimiEdit->text());
    if( !ui->ytunnusEdit->text().isEmpty())
        velho->asetukset_.insert("Ytunnus", ui->ytunnusEdit->text().simplified());

    if( !ui->osoiteEdit->toPlainText().isEmpty())
        velho->asetukset_.insert("Katuosoite", ui->osoiteEdit->toPlainText());
    if( !ui->postinumeroEdit->text().isEmpty())
        velho->asetukset_.insert("Postinumero", ui->postinumeroEdit->text());
    if( !ui->kaupunkiEdit->text().isEmpty())
        velho->asetukset_.insert("Kaupunki", ui->kaupunkiEdit->text());
    if( !ui->kotipaikkaEdit->text().isEmpty())
        velho->asetukset_.insert("Kotipaikka", ui->kotipaikkaEdit->text());
    if( !ui->emailEdit->text().isEmpty())
        velho->asetukset_.insert("Email", ui->emailEdit->text());
    if( !ui->webEdit->text().isEmpty())
        velho->asetukset_.insert("Kotisivu", ui->webEdit->text());
    if( !ui->puhelinEdit->text().isEmpty())
        velho->asetukset_.insert("Puhelin", ui->puhelinEdit->text());

    velho->asetukset_.insert("muoto", ui->muotoList->currentItem()->data(Qt::UserRole).toString());
    velho->asetukset_.insert("laajuus", ui->laajuusList->currentItem()->data(Qt::UserRole).toString());

    // 3.3 beta: VAT-kirjanpidon luominen
    // 5.0-beta.1 Myös jos alustettava kirjanpito
    const bool voiLuodaVatKirjanpidon = kp()->pilvi()->pilvi().vat() || wizard()->startId() == UusiVelho::ALUSTUS;

    if( velho->asetukset_.value("laajuus").toInt() >= velho->asetukset_.value("alvlaajuus").toInt()) {
        if( voiLuodaVatKirjanpidon || !field("pilveen").toBool() ) {
            // AlvVelvollinen vain jos riittävä tilaus
            velho->asetukset_.insert("AlvVelvollinen","ON");
        } else {
            qWarning() << "Tilaus ei oikeuta arvonlisäverovelvollista kirjanpitoa";
        }
    }

    if( IbanValidator::kelpaako(ui->tiliLine->text())) {
        QString iban = ui->tiliLine->text().remove(QRegularExpression("\\W"));
        for(int i=0; i < velho->tilit_.count(); i++) {
            if( velho->tilit_.at(i).toMap().value("tyyppi") == "ARP") {
                QVariantMap map = velho->tilit_.at(i).toMap();                
                map.insert("iban", iban);
                velho->tilit_[i] = map;
                break;
            }
        }
        velho->asetukset_.insert("LaskuIbanit", iban);
    }

    return true;

}

void TiedotSivu::haeytunnarilla()
{
    if( ui->ytunnusEdit->hasAcceptableInput() ) {
        QNetworkRequest request( QUrl("http://avoindata.prh.fi/bis/v1/" + ui->ytunnusEdit->text()));
        QNetworkReply *reply = kp()->networkManager()->get(request);
        connect( reply, &QNetworkReply::finished, this, &TiedotSivu::yTietoSaapuu);
    }
}

void TiedotSivu::yTietoSaapuu()
{
    QNetworkReply *reply = qobject_cast<QNetworkReply*>( sender());
    QVariant var = QJsonDocument::fromJson( reply->readAll() ).toVariant();
    if( var.toMap().value("results").toList().isEmpty())
        return;

    QVariantMap tieto = var.toMap().value("results").toList().value(0).toMap();
    ui->nimiEdit->setText( tieto.value("name").toString() );

    QVariantList osoitteet = tieto.value("addresses").toList();
    for(auto item : osoitteet) {
        QVariantMap osoite = item.toMap();
        if( osoite.value("endDate").toDate().isValid() )
            continue;
        ui->osoiteEdit->setPlainText( osoite.value("street").toString() );
        ui->postinumeroEdit->setText( osoite.value("postCode").toString() );
        ui->kaupunkiEdit->setText( osoite.value("city").toString());
        break;
    }

    QVariantList paikat = tieto.value("registedOffices").toList();
    for( auto item : paikat) {
        QVariantMap paikka = item.toMap();
        if( paikka.value("endDate").toDate().isValid() )
            continue;
        // Tähän voidaan myöhemmin tehdä täsmääminen kielivalinnan kanssa ;)
        QString kotipaikka = paikka.value("name").toString();
        ui->kotipaikkaEdit->setText( kotipaikka.toUpper().left(1) + kotipaikka.toLower().mid(1) );
    }

    QVariantList contactDetails = tieto.value("contactDetails").toList();
    for(auto item : contactDetails) {
        const QVariantMap iMap = item.toMap();
        const QString typeName = iMap.value("type").toString();
        const QString value = iMap.value("value").toString();
        if( typeName == "Kotisivun www-osoite" && ui->webEdit->text().isEmpty()) {
            ui->webEdit->setText( value );
        } else if( typeName == "Puhelin" && ui->puhelinEdit->text().isEmpty()) {
            ui->puhelinEdit->setText( value );
        }
    }

    const QString muoto = tieto.value("companyForm").toString().toLower();
    if( !muoto.isEmpty()) {
        for(int i = 0; i < ui->muotoList->count(); i++) {
            auto item = ui->muotoList->item(i);
            if( item->data(Qt::UserRole).toString() == muoto) {
                ui->muotoList->setCurrentItem(item);
            }
        }
    }

}

void TiedotSivu::haeToimipaikka()
{
    QString toimipaikka = Postinumerot::toimipaikka( ui->postinumeroEdit->text() );
    if( !toimipaikka.isEmpty() )
        ui->kaupunkiEdit->setText(toimipaikka);
}
