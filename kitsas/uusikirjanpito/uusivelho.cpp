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
#include "ui_uusivastuu.h"
#include "ui_varmista.h"
#include "ui_uusialustus.h"

#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonParseError>
#include <QMessageBox>

#include <QDebug>
#include <QPixmap>
#include <QFileDialog>

#include "uusialkusivu.h"
#include "tilikausisivu.h"
#include "sijaintisivu.h"
#include "tiedotsivu.h"

#include "validator/ibanvalidator.h"
#include "validator/ytunnusvalidator.h"

#include "db/kirjanpito.h"
#include "pilvi/pilvimodel.h"

#include "db/tositetyyppimodel.h"
#include "sqlite/sqlitemodel.h"

#include <iostream>
#include "kieli/kielet.h"

#include <QSettings>
#include <QUuid>

UusiVelho::UusiVelho(QWidget *parent) :
    QWizard(parent)
{
    setPixmap( QWizard::LogoPixmap, QPixmap(":/pic/possu64.png")  );


    addPage( new UusiAlkuSivu );
    addPage( new VarmistaSivu );
    addPage( new Harjoitussivu(this) );
    addPage( new VastuuSivu );
    addPage( new UusiAlustus);
    addPage( new Tilikarttasivu(this) );
    addPage( new TiedotSivu(this));
    addPage( new TilikausiSivu(this) );
    addPage( new NumerointiSivu );
    addPage( new SijaintiSivu );
    addPage( new LoppuSivu );

    setOption(HaveHelpButton, true);
    connect(this, &UusiVelho::helpRequested, [] { kp()->ohje("aloittaminen/uusi"); });

}

bool UusiVelho::lataaKartta(const QString &polku)
{
    QVariantMap map = kartta(polku);
    if( map.isEmpty())
        return false;
    QVariantMap asetukset = map.value("asetukset").toMap();
    if( !asetukset.contains("KitsasVersio"))
        return false;
    asetukset_ = asetukset;
    tilit_ = map.value("tilit").toList();
    return true;
}

QVariantMap UusiVelho::kartta(const QString &polku)
{
    QFile kartta( polku );
    if( kartta.open(QIODevice::ReadOnly)) {
        QJsonParseError error;
        QJsonDocument doc = QJsonDocument::fromJson(kartta.readAll(), &error);
        if( error.error ) {
            QMessageBox::critical(nullptr, tr("Tilikartan lukeminen epäonnistui"),
                                  tr("Virheellinen tilikarttatiedosto") + "\n" + error.errorString());
            return QVariantMap();
        } else {
            QVariantMap map = doc.toVariant().toMap();
            return map;
        }
    }
    QMessageBox::critical(nullptr, tr("Tilikartan lukeminen epäonnistui"), tr("Tilikarttatiedostoa %1 ei voi avata").arg(polku));
    return QVariantMap();
}

QVariantMap UusiVelho::data() const
{
    QVariantMap map;
    QVariantMap asetusMap(asetukset_);
    QVariantMap initMap;

    if( field("harjoitus").toBool() )
        asetusMap.insert("Harjoitus", "ON");

    if( field("erisarjaan").toBool())
        asetusMap.insert("erisarjaan", "ON");

    if( field("kateissarjaan").toBool())
            asetusMap.insert("kateissarjaan", "ON");

    asetusMap.insert("KpVersio", SQLiteModel::TIETOKANTAVERSIO );
    asetusMap.insert("LuotuVersiolla", qApp->applicationVersion());
    asetusMap.insert("Luotu", QDateTime::currentDateTime());
    asetusMap.insert("UID", QUuid::createUuid().toString() );

    if( !veroViiteMap_.isEmpty()) {
        // Verojen viitteet valmiiksi asetuksiin
        asetusMap.insert("VeroTuloViite", veroViiteMap_.value("173").toString());
        asetusMap.insert("VeroOmaViite", veroViiteMap_.value("172").toString());

    }

    if( kp()->settings()->value("SmtpServer").toString().isEmpty() &&
        field("pilveen").toBool() ) {
        // Jos sähköpostiasetuksia ei ole määritelty konekohtaisesti,
        // otetaan käyttöön Kitsaan pilvisähköposti
        asetusMap.insert("KitsasEmail", true);
        asetusMap.insert("EmailOsoite", kp()->pilvi()->kayttaja().email() );
        asetusMap.insert("EmailNimi", asetukset_.value("Nimi"));
    }


    initMap.insert("asetukset", asetusMap);
    initMap.insert("tilit", tilit_);
    initMap.insert("tilikaudet", tilikaudet_);

    map.insert("name", asetukset_.value("Nimi"));
    map.insert("trial", field("harjoitus").toBool());
    map.insert("init", initMap);
    if(!field("ytunnus").toString().isEmpty())
        map.insert("businessid", field("ytunnus").toString());


//    std::cout << QJsonDocument::fromVariant(map).toJson(QJsonDocument::Compact).toStdString();

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
    if( currentId() == ALOITUS &&
            field("pilveen").toBool())
        return HARJOITUS;

    if( currentId() == HARJOITUS && field("harjoitus").toBool())
        return TILIKARTTA;

    if( currentId() == VASTUU)
        return TILIKARTTA;

    if( currentId() == NUMEROINTI &&
            field("pilveen").toBool())
        return LOPPU;

    return QWizard::nextId();
}

QVariantMap UusiVelho::asetukset(const QString &polku)
{
    QVariantMap map;

    // tilikartan tiedot
    QFile asetukset(polku + "/tilikartta.json");
    if( asetukset.open(QIODevice::ReadOnly))
        map = QJsonDocument::fromJson( asetukset.readAll() ).toVariant().toMap();

    // json asetuksille
    {
        QFile asetukset(polku + "/asetukset.json");
        if( asetukset.open(QIODevice::ReadOnly))
            map.insert(QJsonDocument::fromJson( asetukset.readAll() ).toVariant().toMap());
    }
    // json-tiedosto raporteille
    {
        QFile raportit(polku + "/raportit.json");
        if( raportit.open(QIODevice::ReadOnly))
            map.insert(QJsonDocument::fromJson( raportit.readAll() ).toVariant().toMap());

    }

    // Tilinpäätöksen pohja on tekstitiedosto, jossa kielet on merkattu []-tageilla
    // Luetaan osaksi asetuksia
    {
        QFile pohja(polku + "/tilinpaatos.txt");
        QString kieli;
        QStringList rivit;
        if( pohja.open(QIODevice::ReadOnly)) {
            QTextStream luku(&pohja);            
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
            luku.setCodec("utf8");
#endif
            while(!luku.atEnd()) {
                QString rivi = luku.readLine();
                if( rivi.startsWith("[") && rivi.endsWith("]")) {
                    if( rivit.count() )
                        map.insert("tppohja/" + kieli, rivit.join('\n'));
                    rivit.clear();
                    kieli=rivi.mid(1, rivi.length()-2);
                } else
                    rivit.append(rivi);
            }
            map.insert("tppohja/" + kieli,
                       rivit.join('\n'));
        }
    }
    return map;
}

QVariantMap UusiVelho::alustusVelho(const QString &ytunnus, const QString &nimi, bool harjoitus)
{
    setField("nimi", nimi);
    setField("ytunnus", ytunnus);
    setField("harjoitus", harjoitus);
    setField("pilveen", true);

    setStartId(ALUSTUS);

    // Haetaan veroviitteet, jotta ne saadaan jo valmiiksi
    // asetuksiin

    QString url = QString("%1/info/refs").arg(kp()->pilvi()->service("vero"));
    KpKysely* kysymys = kp()->pilvi()->kysely(url);
    connect(kysymys, &KpKysely::vastaus, this, &UusiVelho::veroViiteTulos);
    kysymys->kysy();


    if( exec())
        return data();
    else
        return QVariantMap();

}

void UusiVelho::veroViiteTulos(QVariant *data)
{
    veroViiteMap_ = data->toMap();
}




UusiVelho::Harjoitussivu::Harjoitussivu(UusiVelho *wizard) :
    ui( new Ui::UusiHarjoitus),
    velho(wizard)
{
    ui->setupUi(this);
    setTitle(UusiVelho::tr("Harjoitus vai todellinen?"));    
    registerField("harjoitus", ui->harjoitusButton);
}



UusiVelho::Tilikarttasivu::Tilikarttasivu(UusiVelho *wizard) :
    ui ( new Ui::UusiTilikartta),
    velho (wizard)
{
    ui->setupUi(this);
    setTitle( UusiVelho::tr("Tilikartta"));
    connect( ui->tiedostoNappi, &QPushButton::clicked, this, &Tilikarttasivu::tiedostosta );
}

bool UusiVelho::Tilikarttasivu::validatePage()
{
    if( ui->yhdistysButton->isChecked() )
        return velho->lataaKartta(":/tilikartat/yhdistys.kitsaskartta");
    else if(ui->elinkeinoRadio->isChecked())
        return velho->lataaKartta(":/tilikartat/yritys.kitsaskartta");
    else if(ui->asoyButton->isChecked())
        return velho->lataaKartta(":/tilikartat/asoy.kitsaskartta");
    else
        return false;
}

void UusiVelho::Tilikarttasivu::tiedostosta()
{
    QString tiedosto = QFileDialog::getOpenFileName(this, UusiVelho::tr("Valitse tilikarttatiedoston"), QString(),
                                                    UusiVelho::tr("Kitsaan tilikartta (*.kitsaskartta)",".kitsaskartta on tiedostopääte - älä käännä sitä"));
    if( !tiedosto.isEmpty() ) {
        if( velho->lataaKartta(tiedosto) )
            wizard()->next();
    }
}


UusiVelho::NumerointiSivu::NumerointiSivu()
    : ui(new Ui::UusiNumerointi)
{
    ui->setupUi(this);
    setTitle(tr("Tositteiden numerointi"));
    registerField("erisarjaan", ui->erisarjaan);
    registerField("kateissarjaan", ui->kateissarjaan);
}

UusiVelho::VarmistaSivu::VarmistaSivu() :
    ui( new Ui::Varmista)
{
    ui->setupUi(this);
    setTitle(UusiVelho::tr("Huolehdi kirjanpitosi varmuuskopioinnista"));
}

UusiVelho::VastuuSivu::VastuuSivu() :
    ui( new Ui::Uusivastuu)
{
    ui->setupUi(this);
    setTitle(UusiVelho::tr("Vastuu kirjanpidosta"));
}

UusiVelho::LoppuSivu::LoppuSivu() :
    ui( new Ui::UusiLoppu)
{
    ui->setupUi(this);
    setTitle(UusiVelho::tr("Valmis"));
}

void UusiVelho::LoppuSivu::initializePage()
{
    ui->koneLabel->setText(UusiVelho::tr("<p>Kirjanpitosi tallennetaan tietokoneellesi hakemistoon <tt>%1</tt> "
                              "tiedostoon <tt>%2</tt></p>"
                              "<p><b>Muista tiedostosi sijainti</b>, jotta pystyt avaamaan sen myöhemmin.</p>"
                              "<p><b>Huolehdi jatkuvasti tämän tiedoston varmuuskopioinnista!</b>")
                           .arg(field("sijainti").toString())
                           .arg(field("tiedosto").toString()));

    bool pilveen = field("pilveen").toBool();
    ui->pilviLabel->setVisible(pilveen);
    ui->koneLabel->setVisible(!pilveen);

}

UusiVelho::UusiAlustus::UusiAlustus()
    : ui{ new Ui::UusiAlustus}
{
    ui->setupUi(this);
}

void UusiVelho::UusiAlustus::initializePage()
{
    ui->nimiLabel->setText( field("nimi").toString() + "\n" + field("ytunnus").toString() );
}
