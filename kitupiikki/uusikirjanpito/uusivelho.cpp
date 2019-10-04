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
#include "uusivelho.h"

#include "ui_uusiharjoitus.h"
#include "ui_uusitilikartta.h"
#include "ui_uusitiedot.h"
#include "ui_uusiloppu.h"
#include "ui_numerointi.h"

#include "db/kielikentta.h"

#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>

#include <QDebug>
#include <QPixmap>

#include "uusialkusivu.h"
#include "tilikausisivu.h"
#include "sijaintisivu.h"

#include "validator/ibanvalidator.h"
#include "validator/ytunnusvalidator.h"

#include "db/kirjanpito.h"
#include "pilvi/pilvimodel.h"

#include "db/tositetyyppimodel.h"

UusiVelho::UusiVelho()
{
    setPixmap( QWizard::LogoPixmap, QPixmap(":/pic/possu64.png")  );


    addPage( new UusiAlkuSivu );
    addPage( new Harjoitussivu );
    addPage( new Tilikarttasivu(this) );
    addPage( new TiedotSivu(this));
    addPage( new TilikausiSivu(this) );
    addPage( new NumerointiSivu );
    addPage( new SijaintiSivu );

    QWizardPage *loppusivu = new QWizardPage;
    Ui::UusiLoppu *loppuUi = new Ui::UusiLoppu;
    loppuUi->setupUi(loppusivu);
    loppusivu->setTitle( tr("Valmis"));
    addPage( loppusivu );

}

void UusiVelho::lataaKartta(const QString &polku)
{
    QFile tiedosto(polku);
    if( tiedosto.open(QIODevice::ReadOnly)) {
        QByteArray ba = tiedosto.readAll();
        QJsonDocument doc( QJsonDocument::fromJson(ba) );

        qDebug() << ba;
        qDebug() << doc.toJson();

        asetukset_ = doc.object().value("asetukset").toVariant().toMap();
        tilit_ = doc.object().value("tilit").toVariant().toList();
    }

    qDebug() << " Kartta " << polku << " Asetuksia " << asetukset_.count() << " Tilejä " << tilit_.count();
}

QVariantMap UusiVelho::data() const
{
    QVariantMap map;
    QVariantMap asetusMap(asetukset_);
    QVariantMap initMap;

    if( field("harjoitus").toBool())
        asetusMap.insert("harjoitus", true);

    if( field("erisarjaan").toBool()) {
        if( field("kateissarjaan").toBool())
            asetusMap.insert("sarjaan",TositeTyyppiModel::KATEISSARJA);
        else
            asetusMap.insert("sarjaan",TositeTyyppiModel::TOSITELAJIT);
    }

    initMap.insert("asetukset", asetusMap);
    initMap.insert("tilit", tilit_);
    initMap.insert("tilikaudet", tilikaudet_);
    map.insert("name", asetukset_.value("Nimi"));
    map.insert("init", initMap);

    return  map;
}

QString UusiVelho::polku() const
{
    if( field("pilveen").toBool())
        return QString();
    return field("sijainti").toString()+"/"+field("tiedosto").toString();
}

int UusiVelho::nextId() const
{

    if( currentId() == NUMEROINTI &&
            field("pilveen").toBool())
        return LOPPU;

    return QWizard::nextId();
}



UusiVelho::Harjoitussivu::Harjoitussivu() :
    ui( new Ui::UusiHarjoitus)
{
    ui->setupUi(this);
    setTitle(tr("Harjoitus vai todellinen?"));
    registerField("harjoitus", ui->harjoitusButton);
}

UusiVelho::Tilikarttasivu::Tilikarttasivu(UusiVelho *wizard) :
    ui ( new Ui::UusiTilikartta),
    velho (wizard)
{
    ui->setupUi(this);
    setTitle( tr("Tilikartta"));
}

bool UusiVelho::Tilikarttasivu::validatePage()
{
    if( ui->yhdistysButton->isChecked() )
        velho->lataaKartta(":/tilikartat/yhdistys.json");
    else
        return false;   // Tilapäisesti kun ei vielä muita karttoja

    return true;
}

UusiVelho::TiedotSivu::TiedotSivu(UusiVelho *wizard) :
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
}

void UusiVelho::TiedotSivu::initializePage()
{
    // Haetaan muodot
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
    QVariantMap laajuusMap = velho->asetukset_.value("laajuudet").toMap();
    QMapIterator<QString,QVariant> laajuusIter(laajuusMap);
    while( laajuusIter.hasNext()) {
        laajuusIter.next();
        KieliKentta kk( laajuusIter.value() );
        QListWidgetItem *item = new QListWidgetItem(kk.teksti(), ui->laajuusList);
        item->setData(Qt::UserRole, laajuusIter.key());
    }
    ui->laajuusList->setCurrentRow( ui->laajuusList->model()->rowCount() / 2 );
}

bool UusiVelho::TiedotSivu::validatePage()
{
    velho->asetukset_.insert("Nimi", ui->nimiEdit->text());
    if( !ui->ytunnusEdit->text().isEmpty())
        velho->asetukset_.insert("Ytunnus", ui->ytunnusEdit->text());

    velho->asetukset_.insert("muoto", ui->muotoList->currentItem()->data(Qt::UserRole).toString());
    velho->asetukset_.insert("laajuus", ui->laajuusList->currentItem()->data(Qt::UserRole).toString());

    if( velho->asetukset_.value("laajuus").toInt() >= velho->asetukset_.value("alvlaajuus").toInt()) {
        velho->asetukset_.insert("AlvVelvollinen",true);
    }

    if( !ui->tiliLine->text().isEmpty())
        for(int i=0; i < velho->tilit_.count(); i++) {
            if( velho->tilit_.at(i).toMap().value("tyyppi") == "ARP") {
                QVariantMap map = velho->tilit_.at(i).toMap();
                map.insert("IBAN", ui->tiliLine->text());
                velho->tilit_[i] = map;
                break;
            }
    }

    return true;

}


UusiVelho::NumerointiSivu::NumerointiSivu()
    : ui(new Ui::UusiNumerointi)
{
    ui->setupUi(this);
    setTitle(tr("Tositteiden numerointi"));
    registerField("erisarjaan", ui->erisarjaan);
    registerField("kateissarjaan", ui->kateissarjaan);
}
