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
#include "tiedotsivu.h"

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
    // json asetuksille
    {
        QFile asetukset(polku + "/asetukset.json");
        if( asetukset.open(QIODevice::ReadOnly))
            asetukset_ = QJsonDocument::fromJson( asetukset.readAll() ).toVariant().toMap();
    }
    // json-tiedosto raporteille
    {
        QFile raportit(polku + "/raportit.json");
        if( raportit.open(QIODevice::ReadOnly))
            asetukset_.unite( QJsonDocument::fromJson( raportit.readAll()).toVariant().toMap() );
    }
    // Tilit oma json-tiedosto
    {
        QFile tilit(polku + "/tilit.json");
        if( tilit.open(QIODevice::ReadOnly) )
            tilit_ = QJsonDocument::fromJson( tilit.readAll() ).toVariant().toList();
    }

    // Tilinpäätöksen pohja on tekstitiedosto, jossa kielet on merkattu []-tageilla
    // Luetaan osaksi asetuksia
    {
        QFile pohja(polku + "/tilinpaatos.txt");
        QString kieli;
        QStringList rivit;
        if( pohja.open(QIODevice::ReadOnly)) {
            QTextStream luku(&pohja);
            luku.setCodec("utd-8");
            while(!luku.atEnd()) {
                QString rivi = luku.readLine();
                if( rivi.startsWith("[") && rivi.endsWith("]")) {
                    if( rivit.count() )
                        asetukset_.insert("tppohja/" + kieli, rivit);
                    rivit.clear();
                    kieli=rivi.mid(1, rivi.length()-2);
                } else
                    rivit.append(rivi);
            }
            asetukset_.insert("tppohja/" + kieli, rivit.join("\n"));
        }
    }

}

QVariantMap UusiVelho::data() const
{
    QVariantMap map;
    QVariantMap asetusMap(asetukset_);
    QVariantMap initMap;

    if( field("harjoitus").toBool())
        asetusMap.insert("Harjoitus", true);



    if( field("erisarjaan").toBool())
        asetusMap.insert("erisarjaan", true);

    if( field("kateissarjaan").toBool())
            asetusMap.insert("kateissarjaan", true);

    asetusMap.insert("KpVersio", Kirjanpito::TIETOKANTAVERSIO );
    asetusMap.insert("LuotuVersiolla", qApp->applicationVersion());

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
        velho->lataaKartta(":/tilikartat/yhdistys");
    else
        return false;   // Tilapäisesti kun ei vielä muita karttoja

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
