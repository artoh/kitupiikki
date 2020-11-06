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
#include "maksumuistutusdialogi.h"
#include "ui_maksumuistutusdialogi.h"
#include "db/kirjanpito.h"
#include "model/tosite.h"
#include "db/tositetyyppimodel.h"
#include "myyntilaskuntulostaja.h"
#include "model/tositevienti.h"
#include "model/tositeviennit.h"
#include "laskudialogi.h"
#include <QPushButton>

MaksumuistutusDialogi::MaksumuistutusDialogi(QList<int> erat, QWidget *parent) :
    QDialog(parent),
    ui(new Ui::MaksumuistutusDialogi),
    erat_(erat)
{
    ui->setupUi(this);
    ui->eraDate->setDate(MyyntiLaskunTulostaja::erapaiva());
    haeEra();
    setAttribute(Qt::WA_DeleteOnClose);
}

MaksumuistutusDialogi::~MaksumuistutusDialogi()
{
    delete ui;
}

void MaksumuistutusDialogi::accept()
{
    ui->buttonBox->button(QDialogButtonBox::Ok)->setEnabled(false);
    erat_ = muistutettavat_.keys();
    tallennaSeuraava();
}

void MaksumuistutusDialogi::haeEra()
{
    if( erat_.isEmpty()) {
        show();
    } else {
        int era = erat_.takeFirst();
        KpKysely *kysely = kpk(QString("/viennit"));
        kysely->lisaaAttribuutti("era", era);
        connect( kysely, &KpKysely::vastaus, this, &MaksumuistutusDialogi::eraSaapuu);
        kysely->kysy();
    }
}

void MaksumuistutusDialogi::eraSaapuu(QVariant *data)
{
    QVariantList lista = data->toList();

    QVariantMap eramap = lista.value(0).toMap().value("era").toMap();
    int eraid = eramap.value("id").toInt();
    eraMapit_.insert(eraid, eramap);

    for(auto item : lista) {
        QVariantMap lmap = item.toMap();
        int id = lmap.value("tosite").toMap().value("id").toInt();
        KpKysely *kysely = kpk(QString("/tositteet/%1").arg(id));
        connect( kysely, &KpKysely::vastaus,
                 [this, eraid] (QVariant* vastaus) {this->tositeSaapuu(eraid, vastaus);});
        kysely->kysy();
    }
    haeEra();
}

void MaksumuistutusDialogi::tositeSaapuu(int era, QVariant *data)
{
    QVariantList lista = muistutettavat_.value(era);
    QVariantMap map = data->toMap();
    map.remove("loki");
    map.remove("liitteet");

    lista.append(map);
    muistutettavat_.insert(era, lista);
}

QVariantMap MaksumuistutusDialogi::muodostaMuistutus(int era)
{

    Tosite muistutus;
    muistutus.asetaTyyppi(TositeTyyppi::MAKSUMUISTUTUS);
    muistutus.asetaPvm(kp()->paivamaara());
    muistutus.setData(Tosite::TILA, Tosite::VALMISLASKU);
    muistutus.asetaErapvm( ui->eraDate->date());    
    QVariantList tositteet = muistutettavat_.value(era);
    QVariantMap amap = tositteet.value(0).toMap();
    QVariantMap eramap = eraMapit_.value(era);
    int kumppaniId = amap.value("kumppani").toMap().value("id").toInt();

    QVariantMap lasku = amap.value("lasku").toMap();
    MyyntiLaskunTulostaja tulostaja(lasku.value("kieli").toString());
    muistutus.asetaKommentti( tulostaja.t("muistutusteksti") );
    QString otsikko = tulostaja.t("mmotsikko").arg(lasku.value("numero").toString());
    muistutus.asetaOtsikko( otsikko );
    lasku.insert("otsikko", otsikko);

    lasku.insert("alkupNro", lasku.value("numero"));
    lasku.insert("alkupPvm", lasku.value("pvm"));
    lasku.remove("numero");
    lasku.insert("pvm", kp()->paivamaara());
    lasku.insert("erapvm", ui->eraDate->date()); // TODO: Päivämääräjutut!!!
    lasku.insert("aiemmat", tositteet);
    lasku.insert("saate", tulostaja.t("muistutussaate"));
    lasku.insert("maksutapa", LaskuDialogi::LASKU);
    if( lasku.value("laskutapa").toInt() == LaskuDialogi::EITULOSTETA)
        lasku.insert("laskutapa", LaskuDialogi::TULOSTETTAVA);

    if(kumppaniId) {
        muistutus.asetaKumppani( kumppaniId);
    }


    // Tähän pitäisi lisätä rivit ja viennit
    qlonglong kulut = 0;
    QVariantList rivit;
    QVariantList viennit;

    // Maksumuistutus
    if( ui->muistutusCheck->isChecked()) {
        TositeVienti mmvienti;
        mmvienti.setPvm(kp()->paivamaara());
        mmvienti.setTili(kp()->asetukset()->luku("LaskuMaksumuistustili",9170)); // Tämä asetuksiin
        mmvienti.setTyyppi(TositeTyyppi::TULO + TositeVienti::KIRJAUS);
        mmvienti.setKredit(ui->muistutusSpin->value());        
        kulut+=qRound64(ui->muistutusSpin->value() * 100.0);
        viennit.append(mmvienti);

        QVariantMap mmmap;
        mmmap.insert("nimike", tulostaja.t("muistutusmaksu"));   // Tähän käännös
        mmmap.insert("myyntikpl",1);
        mmmap.insert("ahinta", ui->muistutusSpin->value());
        mmmap.insert("tili",kp()->asetukset()->luku("LaskuMaksumuistustili",9170));
        rivit.append(mmmap);

        lasku.insert("muistutusmaksu", ui->muistutusSpin->value());
    }

    // Koron laskeminen. Tässä pitäisi huomioida alkuperäinen eräpäivä ja
    // mahdollinen aiempi korko
    if( ui->korkoCheck->isChecked()) {
        double korko = 0.0;
        QDate korkopaiva;
        for(auto tosite : tositteet) {
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
        // Nyt voidaan laskea viivästyskorko
        if( korko > 1e-5 && korkopaiva.isValid()) {
            double peruste = eramap.value("saldo").toDouble();
            qlonglong paivat = korkopaiva.daysTo(kp()->paivamaara());
            qlonglong vkorkosnt = qRound64( peruste * korko * paivat / kp()->paivamaara().daysInYear() );

            QString selite = tulostaja.t("viivkorko") +
                    QString(" %1 - %2")
                    .arg(korkopaiva.toString("dd.MM.yyyy"))
                    .arg(kp()->paivamaara().toString("dd.MM.yyyy"));


            TositeVienti korkovienti;
            korkovienti.setPvm( kp()->paivamaara());
            korkovienti.setTili(kp()->asetukset()->luku("LaskuViivastyskorkotili",9170));
            korkovienti.setTyyppi(TositeTyyppi::TULO + TositeVienti::KIRJAUS);
            korkovienti.setKredit(vkorkosnt);
            korkovienti.setSelite(selite);
            kulut += vkorkosnt;
            viennit.append(korkovienti);

            QVariantMap komap;
            komap.insert("nimike", selite);
            komap.insert("myyntikpl", paivat);
            komap.insert("ahinta", (1.00 * peruste * korko / kp()->paivamaara().daysInYear() / 100));
            komap.insert("tili", kp()->asetukset()->luku("LaskuViivastyskorkotili",9170));
            rivit.append(komap);

            lasku.insert("korkoalkaa", korkopaiva);
            lasku.insert("korkoloppuu", kp()->paivamaara());
            lasku.insert("korko", vkorkosnt / 100.0);
        }
    }

    TositeVienti vienti;
    vienti.setEra(era);
    vienti.setPvm(kp()->paivamaara());
    vienti.setTili(kp()->tilit()->tiliTyypilla(TiliLaji::MYYNTISAATAVA).numero());
    vienti.setTyyppi(TositeTyyppi::TULO + TositeVienti::VASTAKIRJAUS);
    if(kumppaniId) {
        vienti.setKumppani(kumppaniId);
    }
    vienti.setSelite(muistutus.otsikko());
    vienti.setDebet(kulut);
    viennit.insert(0, vienti);

    lasku.insert("aiempisaldo", eramap.value("saldo"));

    muistutus.setData(Tosite::LASKU, lasku);
    QVariantMap tm = muistutus.tallennettava();
    if( !rivit.isEmpty())
        tm.insert("rivit", rivit);
    tm.insert("viennit", viennit);

    return tm;
}

void MaksumuistutusDialogi::tallennaSeuraava()
{
    if( erat_.isEmpty()) {
        emit kp()->kirjanpitoaMuokattu();
        emit muistutettu();
        QDialog::accept();
    } else {
        QVariantMap muikkari = muodostaMuistutus(erat_.takeFirst());
        KpKysely *kysely = kpk("/tositteet", KpKysely::POST);
        connect(kysely, &KpKysely::vastaus, this, &MaksumuistutusDialogi::tallennaLiite);
        kysely->kysy(muikkari);                
    }
}

void MaksumuistutusDialogi::tallennaLiite(QVariant *data)
{
    // Tallennetaan ensin liite
    QVariantMap map = data->toMap();

    QByteArray liite = MyyntiLaskunTulostaja::pdf( map );
    KpKysely *liitetallennus = kpk( QString("/liitteet/%1/lasku").arg(map.value("id").toInt()), KpKysely::PUT);
    QMap<QString,QString> meta;
    meta.insert("Filename", QString("maksumuistutus%1.pdf").arg( map.value("lasku").toMap().value("numero").toInt() ) );
    connect(liitetallennus, &KpKysely::vastaus, this, &MaksumuistutusDialogi::tallennaSeuraava);
    liitetallennus->lahetaTiedosto(liite, meta);
    merkkaaMuistutetuksi(map);  // Merkitsee aiemmat laskut muistutettu-tilaan
}

void MaksumuistutusDialogi::merkkaaMuistutetuksi(const QVariantMap &data)
{
    QVariantMap muistutettuTila;
    muistutettuTila.insert("tila", Tosite::MUISTUTETTU);    

    // TODOOOO !!!!!

    QVariantList lista = data.value("lasku").toMap().value("aiemmat").toList();
    for(auto item : lista) {
        QVariantMap map = item.toMap();
        if( map.value("tila").toInt() != Tosite::MUISTUTETTU) {
            KpKysely *mkysely = kpk(QString("/tositteet/%1").arg(map.value("id").toInt()), KpKysely::PATCH);
            mkysely->kysy(muistutettuTila);
        }
    }
}
