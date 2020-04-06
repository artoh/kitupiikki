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
#include "alv/alvilmoitustenmodel.h"

#include <QJsonDocument>
#include <QDebug>

Tosite::Tosite(QObject *parent) :
    QObject(parent),
    viennit_(new TositeViennit(this)),
    liitteet_(new TositeLiitteet(this)),
    loki_( new TositeLoki(this))
{
    connect( viennit_, &TositeViennit::dataChanged, this, &Tosite::tarkasta );
    connect( viennit_, &TositeViennit::rowsInserted, this, &Tosite::tarkasta );
    connect( viennit_, &TositeViennit::rowsRemoved, this, &Tosite::tarkasta );
    connect( viennit_, &TositeViennit::modelReset, this, &Tosite::tarkasta );    
    connect( liitteet(), &TositeLiitteet::liitteetTallennettu, this, &Tosite::liitteetTallennettu);
}

QVariant Tosite::data(int kentta) const
{
    return data_.value( avaimet__.at(kentta) );
}

void Tosite::setData(int kentta, QVariant arvo)
{

    if( data_.value( avaimet__.at(kentta)).toString() == arvo.toString() &&
        !data_.value( avaimet__.at(kentta)).toString().isEmpty() ) {
        return;
    }

    // Jos päivämäärä muuttuu toiselle kaudelle, vaihdetaan tositteen tunnistetta
    if( kentta == PVM &&
        kp()->tilikaudet()->tilikausiPaivalle(arvo.toDate()).alkaa() != kp()->tilikaudet()->tilikausiPaivalle( data_.value( avaimet__.at(PVM) ).toDate() ).alkaa())
        setData( Tosite::TUNNISTE, QVariant() );

    if( (arvo.toString().isEmpty() && arvo.type() != QVariant::Map && arvo.type() != QVariant::List) ||
        ( arvo.type() == QVariant::Int && arvo.toInt() == 0) )
        data_.remove( avaimet__.at(kentta) );
    else
        data_.insert( avaimet__.at(kentta), arvo );

    if( kentta == PVM)
        emit pvmMuuttui( arvo.toDate() );
    else if( kentta == OTSIKKO )
        emit otsikkoMuuttui( arvo.toString() );
    else if( kentta == TUNNISTE)
        emit tunnisteMuuttui( arvo.toInt());
    else if( kentta == SARJA)
        emit sarjaMuuttui( arvo.toString());
    else if( kentta == TYYPPI)
        emit tyyppiMuuttui( arvo.toInt());
    else if( kentta == KOMMENTIT)
        emit kommenttiMuuttui( arvo.toString());

    tarkasta();
}

QString Tosite::tilateksti(int tila)
{
    switch (tila) {
    case POISTETTU: return tr("Poistettu");
    case HYLATTY: return tr("Hylätty");
    case SAAPUNUT: return tr("Saapunut");
    case TARKASTETTU: return tr("Tarkastettu");
    case HYVAKSYTTY: return tr("Hyväksytty");
    case LUONNOS: case LASKULUONNOS: return tr("Luonnos");
    case VALMISLASKU: return tr("Lähettämättä");
    case KIRJANPIDOSSA: return tr("Kirjanpidossa");
    case LAHETETAAN: return tr("Lähettäminen epäonnistui");
    case LAHETETTYLASKU: return tr("Lähetetty");
    case MUISTUTETTU: return tr("Muistutettu");
    }
    return QString();
}

QIcon Tosite::tilakuva(int tila)
{
    switch (tila) {
    case POISTETTU: return QIcon(":/pic/roskis.png");
    case HYLATTY: return QIcon(":/pic/sulje.png");
    case SAAPUNUT: return QIcon(":/pic/inbox.png");
    case TARKASTETTU: return QIcon(":/pixaby/tarkastettu.svg");
    case HYVAKSYTTY: return QIcon(":/pixaby/hyvaksytty.svg");
    case LUONNOS: return QIcon(":/pic/harmaa.png");
    case VALMISLASKU: return QIcon(":/pic/keltainen.png");
    case KIRJANPIDOSSA: return QIcon(":/pic/kaytossa.png");
    case LAHETETAAN: return QIcon(":/pic/varoitus.png");
    case LAHETETTYLASKU: return QIcon(":/pic/mail.png");
    case MUISTUTETTU: return QIcon(":/pic/punainenkuori.png");
    }
    return QIcon(":/pic/tyhja.png");
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
    asetaSarja( kp()->tositeTyypit()->sarja( tyyppi , viennit()->rowCount() && kp()->tilit()->tiliNumerolla(viennit()->vienti(0).tili() ).onko(TiliLaji::KATEINEN)));
    setData(TYYPPI, tyyppi);
}

void Tosite::asetaPvm(const QDate &pvm)
{
    if( data_.value("pvm").toDate() != pvm)
        setData(PVM, pvm);
}

void Tosite::asetaKommentti(const QString &kommentti)
{
    setData(KOMMENTIT, kommentti);
}

void Tosite::asetaSarja(const QString &sarja)
{
    setData(SARJA, sarja);
}

void Tosite::asetaKumppani(int id)
{
    QVariantMap kmap;
    kmap.insert("id",id);
    setData(KUMPPANI, kmap);
}

void Tosite::asetaKumppani(const QString &nimi)
{
    QVariantMap kmap;
    kmap.insert("nimi", nimi);
    setData(KUMPPANI, kmap);
}

void Tosite::asetaKumppani(const QVariantMap &map)
{
    setData(KUMPPANI, map);
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

    emit ladattu();

    emit tyyppiMuuttui( tyyppi());
    emit pvmMuuttui( pvm() );
    emit otsikkoMuuttui( otsikko() );
    emit tunnisteMuuttui( tunniste() );
    emit sarjaMuuttui( sarja() );
    emit kommenttiMuuttui( kommentti());


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


    bool muutettu = tallennettu_ != tallennettava() ||
            liitteet()->tallennettaviaLiitteita() ||
            !liitteet()->liitettavat().isEmpty();

    int virheet = 0;    
    if( !pvm().isValid())
        virheet |= PVMPUUTTUU;
    if( pvm() < kp()->tilikaudet()->kirjanpitoAlkaa() ||
        pvm() > kp()->tilikaudet()->kirjanpitoLoppuu())
        virheet |= EIAVOINTAKUTTA;
    else if( pvm() <= kp()->tilitpaatetty())
        virheet |= PVMLUKITTU;

    qlonglong debet = 0;
    qlonglong kredit = 0;

    for(QVariant item : viennit()->viennit().toList()) {
        TositeVienti vienti = item.toMap();
        QDate pvm = vienti.pvm();

        debet += qRound64( vienti.debet() * 100.0 );
        kredit += qRound64( vienti.kredit() * 100.0 );

        if( !kp()->tilit()->tili(vienti.tili()))
            virheet |= Tosite::TILIPUUTTUU;
        if( vienti.alvKoodi() && kp()->alvIlmoitukset()->onkoIlmoitettu(pvm))
            virheet |= Tosite::PVMALV;
        if( !vienti.pvm().isValid())
            virheet |= Tosite::PVMPUUTTUU;

        if( vienti.pvm() > kp()->tilikaudet()->kirjanpitoLoppuu() ||
                vienti.pvm() < kp()->tilikaudet()->kirjanpitoAlkaa())
            virheet |= Tosite::EIAVOINTAKUTTA;
        else if( vienti.pvm() <= kp()->tilitpaatetty())
            virheet |= Tosite::PVMLUKITTU;
    }
    if( debet != kredit)
        virheet |= Tosite::EITASMAA;


    emit tila(muutettu, virheet, debet / 100.0, kredit / 100.0);

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

    emit tyyppiMuuttui( this->tyyppi());
    emit pvmMuuttui( this->pvm() );
    emit otsikkoMuuttui( otsikko() );
    emit tunnisteMuuttui( tunniste() );
    emit sarjaMuuttui( sarja() );
    emit kommenttiMuuttui( kommentti());

    resetointiKaynnissa_ = false;
    tarkasta();

}

void Tosite::tallennusValmis(QVariant *variant)
{
    qDebug() << "Tosite::tallennusValmis";
    QVariantMap map = variant->toMap();
    setData(ID, map.value( avaimet__.at(ID) ).toInt() );
    setData(TUNNISTE, map.value( avaimet__.at(TUNNISTE)).toInt());
    asetaSarja(map.value(avaimet__.at(SARJA)).toString());

    if( liitteet()->tallennettaviaLiitteita())
        liitteet()->tallennaLiitteet( data(ID).toInt() );
    else {
        liitteet()->tallennettu();
        emit talletettu( id(), tunniste(), pvm(),
                         sarja(), tositetila());
        emit kp()->kirjanpitoaMuokattu();
    }
}

void Tosite::tallennuksessaVirhe(int virhe)
{
    emit tallennusvirhe(virhe);
}

void Tosite::liitteetTallennettu()
{
    emit talletettu( id(), tunniste(), pvm(), sarja(), tositetila());
}

QVariantMap Tosite::tallennettava() const
{

    QVariantMap map(data_);
    map.insert("viennit", viennit_->tallennettavat());

    if( map.value("otsikko").toString().isEmpty() && viennit_->rowCount())
        map.insert("otsikko", viennit_->vienti(0).selite());

    map.insert("liita", liitteet_->liitettavat());

    return map;
}


std::map<int,QString> Tosite::avaimet__ = {
    { ID, "id" },
    { PVM, "pvm"},
    { TYYPPI, "tyyppi"},
    { TILA, "tila"},
    { TUNNISTE, "tunniste"},
    { OTSIKKO, "otsikko"},
    { KUMPPANI, "kumppani" },
    { KOMMENTIT, "info"},
    { ALV, "alv"},
    { SARJA, "sarja"},
    { TILIOTE, "tiliote"},
    { LASKU, "lasku"},
    { RIVIT, "rivit"},
    { LASKUPVM, "laskupvm"},
    { ERAPVM, "erapvm"},
    { VIITE, "viite"},
    { KIERTO, "kierto"},
    { PORTAALI, "portaali"}
};
