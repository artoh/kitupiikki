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
#include "db/kielikentta.h"
#include "ui_uusitiedot.h"
#include "validator/ibanvalidator.h"
#include "validator/ytunnusvalidator.h"
#include "rekisteri/postinumerot.h"

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

    connect(ui->postinumeroEdit, &QLineEdit::textEdited, [this] (const QString& numero) { this->ui->kaupunkiEdit->setText( Postinumerot::toimipaikka(numero)); });

}

void TiedotSivu::initializePage()
{
    // Haetaan muodot
    ui->muotoList->clear();
    QVariantMap muotoMap = velho->asetukset_.value("muodot").toMap();
    QMapIterator<QString,QVariant> muotoIter(muotoMap);
    while( muotoIter.hasNext()) {
        muotoIter.next();
        KieliKentta kk( muotoIter.value() );
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
        KieliKentta kk( laajuusIter.value() );
        QListWidgetItem *item = new QListWidgetItem(kk.teksti(), ui->laajuusList);
        item->setData(Qt::UserRole, laajuusIter.key());
    }
    ui->laajuusList->setCurrentRow( velho->asetukset_.value("laajuus").toInt() - 1);

    connect( ui->ytunnusEdit , &QLineEdit::textEdited, this, &TiedotSivu::haeytunnarilla);
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
        velho->asetukset_.insert("Ytunnus", ui->ytunnusEdit->text());

    if( !ui->osoiteEdit->toPlainText().isEmpty())
        velho->asetukset_.insert("Katuosoite", ui->osoiteEdit->toPlainText());
    if( !ui->postinumeroEdit->text().isEmpty())
        velho->asetukset_.insert("Postinumero", ui->postinumeroEdit->text());
    if( !ui->kaupunkiEdit->text().isEmpty())
        velho->asetukset_.insert("Kaupunki", ui->kaupunkiEdit->text());
    if( !ui->kotipaikkaEdit->text().isEmpty())
        velho->asetukset_.insert("Kotipaikka", ui->kotipaikkaEdit->text());

    velho->asetukset_.insert("muoto", ui->muotoList->currentItem()->data(Qt::UserRole).toString());
    velho->asetukset_.insert("laajuus", ui->laajuusList->currentItem()->data(Qt::UserRole).toString());

    if( velho->asetukset_.value("laajuus").toInt() >= velho->asetukset_.value("alvlaajuus").toInt()) {
        velho->asetukset_.insert("AlvVelvollinen",true);
    }

    if( IbanValidator::kelpaako(ui->tiliLine->text())) {
        QString iban = ui->tiliLine->text().remove(QChar(' '));
        for(int i=0; i < velho->tilit_.count(); i++) {
            if( velho->tilit_.at(i).toMap().value("tyyppi") == "ARP") {
                QVariantMap map = velho->tilit_.at(i).toMap();                
                map.insert("IBAN", iban);
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
    QVariantMap osoite = tieto.value("addresses").toList().value(0).toMap();

    ui->osoiteEdit->setPlainText( osoite.value("street").toString() );
    ui->postinumeroEdit->setText( osoite.value("postCode").toString());
    ui->kaupunkiEdit->setText( osoite.value("city").toString());

    ui->kotipaikkaEdit->setText( tieto.value("registedOffices").toList().value(0).toMap().value("name").toString() );

}

void TiedotSivu::haeToimipaikka()
{
    QString toimipaikka = Postinumerot::toimipaikka( ui->postinumeroEdit->text() );
    if( !toimipaikka.isEmpty() )
        ui->kaupunkiEdit->setText(toimipaikka);
}
