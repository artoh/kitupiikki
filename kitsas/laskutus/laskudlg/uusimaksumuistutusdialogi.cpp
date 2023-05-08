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

UusiMaksumuistutusDialogi::UusiMaksumuistutusDialogi(QVariantList muistutuslista, QWidget *parent) :
    QDialog(parent),
    ui(new Ui::MaksumuistutusDialogi),
    lista_(muistutuslista)
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
    nykyinen_++;
    if( nykyinen_ >= lista_.count()) {
        show();
    } else {
        const QVariantMap& map = lista_.at(nykyinen_).toMap();
        int eraid = map.value("eraid").toInt();
        if( eraid > 0) {
            KpKysely *kysely = kpk(QString("/viennit"));
            kysely->lisaaAttribuutti("era", eraid);
            connect( kysely, &KpKysely::vastaus, this, &UusiMaksumuistutusDialogi::eraSaapuu);
            kysely->kysy();
        } else {
            int id = map.value("tosite").toInt();
            tositteita_ = 1;
            KpKysely *kysely = kpk(QString("/tositteet/%1").arg(id));

            connect( kysely, &KpKysely::vastaus, this,
                     [this] (QVariant* vastaus) {this->tositeSaapuu( vastaus);});
            kysely->kysy();
        }
    }
}

void UusiMaksumuistutusDialogi::eraSaapuu(QVariant *data)
{
    QVariantList lista = data->toList();    

    QVariantMap lmap = lista_.at(nykyinen_).toMap();
    lmap.insert("viennit", lista);
    lista_[nykyinen_] = lmap;
    tositteita_ = lista.count();

    if( !tositteita_) {
        kaynnista();
    } else {
        for(auto &item : lista) {
            QVariantMap lmap = item.toMap();
            int id = lmap.value("tosite").toMap().value("id").toInt();
            KpKysely *kysely = kpk(QString("/tositteet/%1").arg(id));

            connect( kysely, &KpKysely::vastaus, this,
                     [this] (QVariant* vastaus) {this->tositeSaapuu( vastaus);});
            kysely->kysy();
        }
    }
}


void UusiMaksumuistutusDialogi::tositeSaapuu( QVariant *data)
{        
    QVariantMap lmap = lista_.at(nykyinen_).toMap();
    QVariantList tositteet = lmap.value("tositteet").toList();

    QVariantMap map = data->toMap();
    map.remove("loki");
    map.remove("liitteet");

    QVariantMap lasku = map.value("lasku").toMap();
    if( lasku.value("toistuvanerapaiva").toInt()) {
        // Kuukausittainen lasku, tämä siivotaan yksittäiseksi
        // ja merkitään eräpäivä
        lasku.remove("toistuvanerapaiva");
        lmap.insert("laskunosa", true);
        int vientiId = lmap.value("vienti").toInt();

        for(const QVariant& vienti : map.value("viennit").toList()) {
            QVariantMap vMap = vienti.toMap();
            if( vientiId == vMap.value("id").toInt()) {
                lasku.insert("erapvm", vMap.value("pvm"));
                break;
            }
        }
        map.insert("lasku", lasku);
    }

    tositteet.append(map);
    lmap.insert("tositteet", tositteet);

    lista_[nykyinen_] = lmap;

    tositteita_--;
    if( !tositteita_)
        kaynnista();

}

void UusiMaksumuistutusDialogi::accept()
{
    ui->buttonBox->button(QDialogButtonBox::Ok)->setEnabled(false);
    nykyinen_ = -1;
    tallennaSeuraava();
}

void UusiMaksumuistutusDialogi::tallennaSeuraava()
{
    nykyinen_++;
    if( nykyinen_ == lista_.count()) {
        emit kp()->kirjanpitoaMuokattu();
        emit muistutettu();
        QDialog::accept();
    } else {
        tallennaMuistutus();
    }
}

void UusiMaksumuistutusDialogi::tallennaMuistutus()
{

    const QVariantMap& nMap = lista_.at(nykyinen_).toMap();

    Tosite* muistutus = new Tosite(this);

    muistutus->asetaTyyppi(TositeTyyppi::MAKSUMUISTUTUS);
    muistutus->asetaSarja( kp()->tositeTyypit()->sarja( TositeTyyppi::MAKSUMUISTUTUS, false) ) ;
    muistutus->asetaPvm(kp()->paivamaara());
    muistutus->asetaErapvm( ui->eraDate->date());

    QVariantList tositteet = nMap.value("tositteet").toList();

    Tosite alkuperainenTosite;
    alkuperainenTosite.lataa( tositteet.value(0).toMap() );
    Lasku lasku = alkuperainenTosite.constLasku();
    const QString& kieli = lasku.kieli().toLower();
    const int vastatili = alkuperainenTosite.viennit()->vienti(0).tili();

    muistutus->asetaKommentti( tulkkaa("muistutusteksti", kieli) );
    QString otsikko = tulkkaa("mmotsikko", kieli).arg(lasku.numero());

    muistutus->asetaOtsikko( otsikko );
    lasku.setOtsikko(otsikko);
    muistutus->asetaKumppani( alkuperainenTosite.kumppani() );

    lasku.unset("jaksopvm");
    lasku.setToimituspvm( kp()->paivamaara() );

    lasku.setAlkuperaisNumero( lasku.numero().toLongLong() );
    lasku.setNumero(QString());
    lasku.setAlkuperaisPvm( lasku.laskunpaiva() );
    lasku.setLaskunpaiva( kp()->paivamaara() );
    lasku.setErapaiva( ui->eraDate->date() );
    lasku.setAiemmat( tositteet );    
    lasku.setLahetystapa( lasku.lahetystapa() == Lasku::EITULOSTETA ?
                                          Lasku::TULOSTETTAVA :
                                          lasku.lahetystapa());

    lasku.setErittely(QStringList());

    Monikielinen saateOtsikko( kp()->asetukset()->asetus("Laskuteksti/Maksumuistutussaate_otsikko") ) ;
    Monikielinen saateSisalto( kp()->asetukset()->asetus("Laskuteksti/Maksumuistutussaate"));
    Monikielinen lisatieto( kp()->asetukset()->asetus("Laskuteksti/Maksumuistutuslisatiedot"));

    lasku.setSaate( saateSisalto.kaannos(kieli) );
    lasku.setSaateOtsikko( saateOtsikko.kaannos(kieli));
    lasku.setLisatiedot( lisatieto.kaannos(kieli));

    Euro saldo =  nMap.value("era").toMap().contains("saldo") ?
            Euro( tositteet.value(0).toMap().value("era").toMap().value("saldo").toString() ) :
            Euro( nMap.value("avoin").toString() );
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
                                 nMap.value("eraid").toInt(),
                                 ui->muistutusCheck->isChecked() ? Euro::fromDouble( ui->muistutusSpin->value()) : Euro(0),
                                 saldo,
                                 korkopaiva,
                                 kp()->paivamaara(),
                                 ui->korkoCheck->isChecked() ? korko : 0.0,
                                 vastatili
                                 );


    if( nMap.contains("laskunosa") ) {
        // Jos yhdestä osasta tullut muikkari, ei voida merkata koko tositetta muistutetuksi
        connect( muistutus, &Tosite::laskuTallennettu, this, &UusiMaksumuistutusDialogi::tallennaSeuraava );
    } else {
        connect( muistutus, &Tosite::laskuTallennettu, this, &UusiMaksumuistutusDialogi::merkkaaMuistutetuksi );
    }
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
