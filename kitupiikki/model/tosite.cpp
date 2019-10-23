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
#include "tosite.h"

#include "tositeviennit.h"
#include "tositeliitteet.h"
#include "tositeloki.h"
#include "db/kirjanpito.h"
#include "db/tositetyyppimodel.h"

#include <QJsonDocument>
#include <QDebug>

Tosite::Tosite(QObject *parent) :
    QObject(parent),
    viennit_(new TositeViennit(this)),
    liitteet_(new TositeLiitteet(this)),
    loki_( new TositeLoki(this))
{
    connect( viennit_, &TositeViennit::dataChanged, this, &Tosite::tarkasta );
    connect( viennit_, &TositeViennit::modelReset, this, &Tosite::tarkasta );    
    connect( liitteet(), &TositeLiitteet::liitteetTallennettu, this, &Tosite::liitteetTallennettu);

}

QVariant Tosite::data(int kentta) const
{
    return data_.value( avaimet__.at(kentta) );
}

void Tosite::setData(int kentta, QVariant arvo)
{

    if( (arvo.toString().isEmpty() && arvo.type() != QVariant::Map) ||
        ( arvo.type() == QVariant::Int && arvo.toInt() == 0) )
        data_.remove( avaimet__.at(kentta) );
    else
        data_.insert( avaimet__.at(kentta), arvo );

    if( kentta == PVM)
        emit pvmMuuttui( arvo.toDate() );
    else if( kentta == OTSIKKO )
        emit otsikkoMuuttui( arvo.toString() );

    tarkasta();
}

QString Tosite::tilateksti(int tila)
{
    switch (tila) {
    case POISTETTU: return tr("Poistettu");
    case LUONNOS: return tr("Luonnos");
    case VALMISLASKU: return tr("Lähettämättä");
    case KIRJANPIDOSSA: return tr("Kirjanpidossa");
    case LAHETETTYLASKU: return tr("Lähetetty");
    }
    return QString();
}

QDate Tosite::pvm() const
{
    return data(PVM).toDate();
}

void Tosite::asetaOtsikko(const QString &otsikko)
{
    setData(OTSIKKO, otsikko);
}

void Tosite::asetaTyyppi(int tyyppi)
{
    setData(TYYPPI, tyyppi);
    setData(SARJA, kp()->tositeTyypit()->sarja( tyyppi ));
}

void Tosite::asetaPvm(const QDate &pvm)
{
    setData(PVM, pvm);
}

void Tosite::lataa(int tositeid)
{
    KpKysely *kysely = kpk(QString("/tositteet/%1").arg(tositeid));
    connect(kysely, &KpKysely::vastaus, this, &Tosite::lataaData);
    kysely->kysy();
}

void Tosite::lataaData(QVariant *variant)
{


    resetointiKaynnissa_ = true;
    data_ = variant->toMap();

    viennit()->asetaViennit( data_.take("viennit").toList() );
    loki()->lataa( data_.take("loki").toList());
    liitteet()->lataa( data_.take("liitteet").toList());

    int kumppani = data_.value("kumppani").toMap().value("id").toInt();
    if( kumppani )
        data_.insert("kumppani", kumppani);

    emit ladattu();
    tallennettu_ = tallennettava();
    resetointiKaynnissa_ = false;

    tarkasta();

}

void Tosite::tallenna(int tilaan)
{
    setData( TILA, tilaan );

    KpKysely* kysely;
    if( data(ID).isNull())
        kysely = kpk( "/tositteet/", KpKysely::POST);
    else
        kysely = kpk( QString("/tositteet/%1").arg( data(ID).toInt() ), KpKysely::PUT);


    connect(kysely, &KpKysely::vastaus, this, &Tosite::tallennusValmis  );
    connect(kysely, &KpKysely::virhe, this, &Tosite::tallennuksessaVirhe);

    kysely->kysy( tallennettava() );

}

void Tosite::tarkasta()
{
    if( resetointiKaynnissa_)
        return;


    bool muutettu = tallennettu_ != tallennettava();

    int virheet = 0;
    double debet = 0.0;
    double kredit = 0.0;

    // Tarkasta päivämäärät ja alvit

    for(int i=0; i < viennit()->rowCount(); i++) {
        debet += viennit()->data(viennit()->index(i, TositeViennit::DEBET), Qt::EditRole).toDouble();
        kredit += viennit()->data(viennit()->index(i, TositeViennit::KREDIT), Qt::EditRole).toDouble();
        if( viennit()->data( viennit()->index(i, TositeViennit::TILI), Qt::EditRole ).toInt() == 0)
            virheet |= TILIPUUTTUU;
    }
    if( qAbs(debet-kredit) > 1e-5 )
        virheet |= EITASMAA;
    if( qAbs(debet)  < 1e-5 && qAbs(kredit) < 1e-5)
        virheet |= NOLLA;

    emit tila(muutettu, virheet, debet, kredit);

}

void Tosite::nollaa(const QDate &pvm, int tyyppi)
{
    resetointiKaynnissa_ = true;
    data_.clear();
    viennit_->asetaViennit(QVariantList());
    liitteet()->clear();
    data_.insert( avaimet__.at(PVM), pvm );
    data_.insert( avaimet__.at(TYYPPI), tyyppi);
    emit ladattu();

    tallennettu_ = tallennettava();
    resetointiKaynnissa_ = false;
    tarkasta();

}

void Tosite::tallennusValmis(QVariant *variant)
{
    QVariantMap map = variant->toMap();
    setData(ID, map.value( avaimet__.at(ID) ).toInt() );
    setData(TUNNISTE, map.value( avaimet__.at(TUNNISTE)).toInt());
    setData(SARJA, map.value(avaimet__.at(SARJA)).toString());

    if( liitteet()->tallennettaviaLiitteita())
        liitteet()->tallennaLiitteet( data(ID).toInt() );
    else
        emit talletettu( data(ID).toInt(), data(TUNNISTE).toInt(), tallennettu_.value( avaimet__.at(PVM) ).toDate(),
                         data(SARJA).toString());

    // Tämä pitää oikaista vielä huomioimaan tilinavauksen sikäli jos sitä tositteen kautta käsitellään ;)
    if( !kp()->asetukset()->onko("EkaTositeKirjattu"))
        kp()->asetukset()->aseta("EkaTositeKirjattu", true);
}

void Tosite::tallennuksessaVirhe(int virhe)
{
    emit tallennusvirhe(virhe);
}

void Tosite::liitteetTallennettu()
{
    emit talletettu( data(ID).toInt(), data(TUNNISTE).toInt(), tallennettu_.value( avaimet__.at(PVM) ).toDate(),
                     data(SARJA).toString());
}

QVariantMap Tosite::tallennettava() const
{

    QVariantMap map(data_);
    map.insert("viennit", viennit_->tallennettavat());

    if( map.value("otsikko").toString().isEmpty() && viennit_->rowCount())
        map.insert("otsikko", viennit_->vienti(0).selite());

    return map;
}


std::map<int,QString> Tosite::avaimet__ = {
    { ID, "id" },
    { PVM, "pvm"},
    { TYYPPI, "tyyppi"},
    { TILA, "tila"},
    { TUNNISTE, "tunniste"},
    { OTSIKKO, "otsikko"},
    { VIITE, "viite"},
    { ERAPVM, "erapvm"},
    { KUMPPANI, "kumppani" },
    { KOMMENTIT, "info"},
    { ALV, "alv"},
    { SARJA, "sarja"}
};
