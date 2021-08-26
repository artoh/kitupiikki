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
#include "uusimaksumuistutusdialogi.h"
#include "ui_uusimaksumuistutusdialogi.h"
#include "db/kirjanpito.h"
#include "model/tosite.h"
#include "db/tositetyyppimodel.h"
#include <QPushButton>
#include "model/lasku.h"
#include "../tulostus/laskuntulostaja.h"
#include "maksumuistutusmuodostaja.h"
#include "model/tositeviennit.h"
#include "model/tositevienti.h"

UusiMaksumuistutusDialogi::UusiMaksumuistutusDialogi(QList<int> erat, QWidget *parent) :
    QDialog(parent),
    ui(new Ui::MaksumuistutusDialogi),
    erat_(erat)
{
    ui->setupUi(this);
    ui->eraDate->setDate( Lasku::oikaiseErapaiva( kp()->paivamaara().addDays( kp()->asetukset()->luku(AsetusModel::LaskuMaksuaika) ) ) );
    setAttribute(Qt::WA_DeleteOnClose);
}

UusiMaksumuistutusDialogi::~UusiMaksumuistutusDialogi()
{
    delete ui;
}

void UusiMaksumuistutusDialogi::kaynnista()
{
    if( erat_.isEmpty()) {
        show();
    } else {
        int era = erat_.takeFirst();
        KpKysely *kysely = kpk(QString("/viennit"));
        kysely->lisaaAttribuutti("era", era);
        connect( kysely, &KpKysely::vastaus, this, &UusiMaksumuistutusDialogi::eraSaapuu);
        kysely->kysy();
    }
}

void UusiMaksumuistutusDialogi::eraSaapuu(QVariant *data)
{
    QVariantList lista = data->toList();

    QVariantMap eramap = lista.value(0).toMap().value("era").toMap();
    int eraid = eramap.value("id").toInt();
    eraMapit_.insert(eraid, eramap);

    for(auto &item : lista) {
        QVariantMap lmap = item.toMap();
        int id = lmap.value("tosite").toMap().value("id").toInt();
        KpKysely *kysely = kpk(QString("/tositteet/%1").arg(id));
        connect( kysely, &KpKysely::vastaus, this,
                 [this, eraid] (QVariant* vastaus) {this->tositeSaapuu(eraid, vastaus);});
        kysely->kysy();
    }
    kaynnista();
}

void UusiMaksumuistutusDialogi::tositeSaapuu(int era, QVariant *data)
{
    QVariantList lista = muistutettavat_.value(era);
    QVariantMap map = data->toMap();
    map.remove("loki");
    map.remove("liitteet");

    lista.append(map);
    muistutettavat_.insert(era, lista);
}

void UusiMaksumuistutusDialogi::accept()
{
    ui->buttonBox->button(QDialogButtonBox::Ok)->setEnabled(false);
    erat_ = muistutettavat_.keys();
    tallennaSeuraava();
}

void UusiMaksumuistutusDialogi::tallennaSeuraava()
{
    if( erat_.isEmpty()) {
        emit kp()->kirjanpitoaMuokattu();
        emit muistutettu();
        QDialog::accept();
    } else {
        tallennaMuistutus( erat_.takeFirst() );
    }
}

void UusiMaksumuistutusDialogi::tallennaMuistutus(int era)
{

    Tosite* muistutus = new Tosite(this);

    muistutus->asetaTyyppi(TositeTyyppi::MAKSUMUISTUTUS);
    muistutus->asetaPvm(kp()->paivamaara());
    muistutus->asetaErapvm( ui->eraDate->date());

    QVariantList tositteet = muistutettavat_.value(era);

    Tosite alkuperainenTosite;
    alkuperainenTosite.lataa( tositteet.value(0).toMap() );
    Lasku lasku = alkuperainenTosite.constLasku();
    const QString& kieli = lasku.kieli().toLower();
    const int vastatili = alkuperainenTosite.viennit()->vienti(0).tili();

    QVariantMap eramap = eraMapit_.value(era);    

    muistutus->asetaKommentti( tulkkaa("muistutusteksti", kieli) );
    QString otsikko = tulkkaa("mmotsikko", kieli).arg(lasku.numero());

    muistutus->asetaOtsikko( otsikko );
    lasku.setOtsikko(otsikko);
    muistutus->asetaKumppani( alkuperainenTosite.kumppani() );

    lasku.setAlkuperaisNumero( lasku.numero().toLongLong() );
    lasku.setNumero(QString());
    lasku.setAlkuperaisPvm( lasku.laskunpaiva() );
    lasku.setLaskunpaiva( kp()->paivamaara() );
    lasku.setErapaiva( ui->eraDate->date() );
    lasku.setAiemmat( tositteet );
    lasku.setSaate( tulkkaa("muistutussaate", kieli) );
    lasku.setLahetystapa( lasku.lahetystapa() == Lasku::EITULOSTETA ?
                                          Lasku::TULOSTETTAVA :
                                          lasku.lahetystapa());


    lasku.setErittely(QStringList());
    lasku.setLisatiedot( tulkkaa("muistutusteksti", kieli) );

    Euro saldo = Euro::fromVariant(eramap.value("saldo"));
    lasku.setAiempiSaldo( saldo );
    muistutus->lasku().kopioi(lasku);

    double korko = 0.0;
    QDate korkopaiva;
    for(auto &tosite : tositteet) {
        QVariantMap map = tosite.toMap();
        int tositetyyppi = map.value("tyyppi").toInt();
        QVariantMap lasku = map.value("lasku").toMap();
        QDate pvm;
        if( tositetyyppi == TositeTyyppi::MYYNTILASKU)
            pvm = lasku.value("erapvm").toDate();
        else if( tositetyyppi == TositeTyyppi::MAKSUMUISTUTUS)
            pvm = lasku.value("pvm").toDate();
        if( korkopaiva.isNull() || pvm > korkopaiva)
            korkopaiva = pvm.addDays(1);
        if( lasku.contains("viivkorko"))
            korko = lasku.value("viivkorko").toDouble();
    }

    MaksumuistutusMuodostaja maksut(kp());
    maksut.muodostaMuistutukset( muistutus,
                                 kp()->paivamaara(),
                                 era,
                                 ui->muistutusCheck->isChecked() ? Euro::fromDouble( ui->muistutusSpin->value()) : Euro(0),
                                 saldo,
                                 korkopaiva,
                                 kp()->paivamaara(),
                                 ui->korkoCheck->isChecked() ? korko : 0.0,
                                 vastatili
                                 );


    connect( muistutus, &Tosite::laskuTallennettu, this, &UusiMaksumuistutusDialogi::merkkaaMuistutetuksi );
    muistutus->tallennaLasku( Tosite::VALMISLASKU );
}


void UusiMaksumuistutusDialogi::merkkaaMuistutetuksi(const QVariantMap &data)
{
    QVariantMap muistutettuTila;
    muistutettuTila.insert("tila", Tosite::MUISTUTETTU);    


    QVariantList lista = data.value("lasku").toMap().value("aiemmat").toList();
    for(const auto& item :  qAsConst( lista )) {
        QVariantMap map = item.toMap();
        if( map.value("tila").toInt() != Tosite::MUISTUTETTU) {
            KpKysely *mkysely = kpk(QString("/tositteet/%1").arg(map.value("id").toInt()), KpKysely::PATCH);
            mkysely->kysy(muistutettuTila);
        }
    }
    tallennaSeuraava();
}
