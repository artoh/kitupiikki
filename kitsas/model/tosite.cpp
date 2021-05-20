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
#include "tositerivit.h"

#include "db/kirjanpito.h"
#include "db/tositetyyppimodel.h"
#include "alv/alvilmoitustenmodel.h"

#include "laskutus/tulostus/laskuntulostaja.h"

#include <QJsonDocument>
#include <QDebug>
#include <QTimer>
#include <QMessageBox>

Tosite::Tosite(QObject *parent) :
    QObject(parent),
    viennit_(new TositeViennit(this)),
    liitteet_(new TositeLiitteet(this)),
    loki_( new TositeLoki(this)),
    rivit_( new TositeRivit(this))
{
    connect( viennit_, &TositeViennit::dataChanged, this, &Tosite::tarkasta );
    connect( viennit_, &TositeViennit::rowsInserted, this, &Tosite::tarkasta );
    connect( viennit_, &TositeViennit::rowsRemoved, this, &Tosite::tarkasta );
    connect( viennit_, &TositeViennit::modelReset, this, &Tosite::tarkasta );    
    connect( liitteet(), &TositeLiitteet::liitteetTallennettu, this, &Tosite::liitteetTallennettu, Qt::QueuedConnection);
}


QVariant Tosite::data(int kentta) const
{
    return data_.value( avaimet__.at(kentta) );
}

void Tosite::setData(int kentta, QVariant arvo)
{

    if( data_.value( avaimet__.at(kentta)) == arvo ||
         (data_.value( avaimet__.at(kentta)).toString() == arvo.toString() &&
        !data_.value( avaimet__.at(kentta)).toString().isEmpty()) ) {
        return;
    }

    // Jos päivämäärä muuttuu toiselle kaudelle, vaihdetaan tositteen tunnistetta
    if( kentta == PVM &&
        kp()->tilikaudet()->tilikausiPaivalle(arvo.toDate()).alkaa() != kp()->tilikaudet()->tilikausiPaivalle( data_.value( avaimet__.at(PVM) ).toDate() ).alkaa())
        setData( Tosite::TUNNISTE, QVariant() );

    if( ((arvo.toString().isEmpty() || arvo.toString()=="0") && arvo.type() != QVariant::Map && arvo.type() != QVariant::List) ||
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
    else if( kentta == LISATIEDOT)
        emit kommenttiMuuttui( arvo.toString());
    else if( kentta == ERAPVM)
        emit eraPvmMuuttui( arvo.toDate());

    tarkasta();
}

QString Tosite::tilateksti(int tila)
{
    switch (tila) {
    case POISTETTU: return tulkkaa("Poistettu");
    case HYLATTY: return tulkkaa("Hylätty");
    case SAAPUNUT: return tulkkaa("Saapunut");
    case TARKASTETTU: return tulkkaa("Tarkastettu");
    case HYVAKSYTTY: return tulkkaa("Hyväksytty");
    case LUONNOS: return tulkkaa("Luonnos");
    case VALMISLASKU: return tulkkaa("Lähettämättä");
    case KIRJANPIDOSSA: return tulkkaa("Kirjanpidossa");
    case LAHETETAAN: return tulkkaa("Lähetetään");
    case LAHETETTYLASKU: return tulkkaa("Lähetetty");
    case MUISTUTETTU: return tulkkaa("Muistutettu");
    case TOIMITETTULASKU: return tulkkaa("Toimitettu");
    case LAHETYSVIRHE: return tulkkaa("Epäonnistui");
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
    case TOIMITETTULASKU: return QIcon(":/pic/mailok.png");
    case LAHETYSVIRHE: return QIcon(":/pic/mailfail.png");
    }
    return QIcon(":/pic/tyhja.png");
}

QDate Tosite::pvm() const
{
    return data(PVM).toDate();
}

void Tosite::asetaOtsikko(const QString &otsikko)
{
    setData(OTSIKKO, otsikko.trimmed());
}

void Tosite::asetaTyyppi(int tyyppi)
{
    // asetaSarja( kp()->tositeTyypit()->sarja( tyyppi , viennit()->rowCount() && kp()->tilit()->tiliNumerolla(viennit()->vienti(0).tili() ).onko(TiliLaji::KATEINEN)));
    setData(TYYPPI, tyyppi);
}

void Tosite::asetaPvm(const QDate &paivamaara)
{
    if( pvm() != paivamaara) {
        for(int i=0; i < viennit_->rowCount(); i++) {
            if( viennit_->vienti(i).pvm() == pvm()) {
                TositeVienti vienti = viennit_->vienti(i);
                vienti.setPvm(paivamaara);
                viennit_->asetaVienti(i, vienti);
            }
        }
        setData(PVM, paivamaara);
    }
}

void Tosite::asetaKommentti(const QString &kommentti)
{
    setData(LISATIEDOT, kommentti);
}

void Tosite::asetaSarja(const QString &sarja)
{
    setData(SARJA, sarja);
}

void Tosite::asetaKumppani(int id)
{
    if(id>0) {
        QVariantMap kmap;
        kmap.insert("id",id);
        setData(KUMPPANI, kmap);
    } else {
        data_.remove(avaimet__.at(KUMPPANI));
    }
}

void Tosite::asetaKumppani(const QString &nimi)
{
    if(nimi.isEmpty()) {
        data_.remove(avaimet__.at(KUMPPANI));
    } else {
        QVariantMap kmap;
        kmap.insert("nimi", nimi);
        setData(KUMPPANI, kmap);
    }
}

void Tosite::asetaKumppani(const QVariantMap &map)
{
    setData(KUMPPANI, map);
}

void Tosite::asetaHuomio(bool onko)
{
    if(onko)
        data_.insert(avaimet__.at(HUOMIO), true);
    else
        data_.remove(avaimet__.at(HUOMIO));
    emit huomioMuuttui(onko);
    tarkasta();
}

void Tosite::asetaTilioterivi(int rivi)
{
    setData(TILIOTERIVI, rivi);
}

void Tosite::pohjaksi(const QDate &paiva, const QString &uusiotsikko, bool sailytaerat)
{
    int siirto = pvm().daysTo(paiva);

    viennit_->pohjaksi(paiva, otsikko(), uusiotsikko, sailytaerat);
    liitteet_->clear();
    loki_->lataa(QVariantList());

    setData(ID, 0);
    setData(TILA, 0);
    setData(TUNNISTE, 0);    
    asetaOtsikko(uusiotsikko);
    asetaPvm(paiva);
    if( erapvm().isValid())
        asetaErapvm(erapvm().addDays(siirto));
}

void Tosite::asetaLaskuNumero(const QString &laskunumero)
{
    QVariantMap map = data_.value("lasku").toMap();
    if( map.value("numero").toString() != laskunumero) {
        if(laskunumero.isEmpty())
            map.remove("numero");
        else
            map.insert("numero", laskunumero);
        data_.insert("lasku", map);
        tarkasta();
    }
}

QString Tosite::laskuNumero() const
{
    return data_.value("lasku").toMap().value("numero").toString();
}

void Tosite::lataa(int tositeid)
{
    KpKysely *kysely = kpk(QString("/tositteet/%1").arg(tositeid));
    connect(kysely, &KpKysely::vastaus, this, &Tosite::lataaData);
    kysely->kysy();
}

void Tosite::lataa(const QVariantMap &map)
{
    resetointiKaynnissa_ = true;
    tallennusKaynnissa_ = false;
    data_ = map;
    tallennettu_.clear();

    // Kun poistettu tunniste palautetaan, ei sillä voi olla tunnistetta
    // jotta ei mene päällekkäin
    if( tila() == Tila::POISTETTU) {
        data_.remove("tunniste");
        tallennettu_.insert("palautettu",QVariant());
    }

    QVariantList vientilista = data_.take("viennit").toList();

    loki()->lataa( data_.take("loki").toList());
    liitteet()->lataa( data_.take("liitteet").toList());
    rivit()->lataa( data_.take("rivit").toList());
    lasku_ = Lasku( data_.take("lasku").toMap());


    if( data(TILA).toInt() == MALLIPOHJA) {
        // Mallipohjasta avattaessa vaihdetaan päivämääräksi nykyinen
        if( erapvm().isValid()) {
            int maksuaika = pvm().daysTo(erapvm());
            asetaErapvm( kp()->paivamaara().addDays(maksuaika));
        }
        asetaPvm( kp()->paivamaara() );
        asetaLaskupvm( kp()->paivamaara() );
        for(int i=0; i < vientilista.count(); i++)
        {
            TositeVienti vienti = vientilista.at(i).toMap();
            vienti.setPvm( kp()->paivamaara() );
            if( vienti.eraId() == vienti.id() ) {
                vienti.setEra(-1);
            }
            vienti.setId(0);
            vientilista[i] = vienti;
        }
    }
    viennit()->asetaViennit( vientilista );
    emit ladattu();

    emit tyyppiMuuttui( tyyppi());
    emit pvmMuuttui( pvm() );
    emit otsikkoMuuttui( otsikko() );
    emit sarjaMuuttui( sarja() );
    emit tunnisteMuuttui( tunniste() );
    emit kommenttiMuuttui( kommentti());
    emit huomioMuuttui( huomio());

    QTimer::singleShot(100, this, &Tosite::latausValmis);
    if( tila() != Tila::POISTETTU) {
        QTimer::singleShot(250, this, &Tosite::laitaTalteen);
    }

    kp()->odotusKursori(false);
}

void Tosite::lataaData(QVariant *variant)
{
    lataa( variant->toMap());
}

void Tosite::tallennaLasku(int tilaan)
{
    asetaTila( tilaan );

    KpKysely* kysely;
    if( !id() )
        kysely = kpk( "/tositteet/", KpKysely::POST);
    else
        kysely = kpk( QString("/tositteet/%1").arg( id() ), KpKysely::PUT);

    connect(kysely, &KpKysely::vastaus, this, &Tosite::tallennaLaskuliite  );
    connect(kysely, &KpKysely::virhe, this, &Tosite::tallennusvirhe);

    kysely->kysy( tallennettava() );
}


void Tosite::tallenna(int tilaan)
{
    if( tallennusKaynnissa_)
        return;
    tallennusKaynnissa_ = true;

    if( data(TILA).toInt() == MALLIPOHJA && tilaan != MALLIPOHJA) {
        setData(ID,QVariant());
    }

    setData( TILA, tilaan );
    if( tilaan >= Tosite::KIRJANPIDOSSA && !viennit_->debetKreditTasmaa()) {
        QMessageBox::critical(nullptr, tr("Virheellinen tosite"),
                              tr("Tositteen debet ja kredit eivät täsmää"));
        tallennuksessaVirhe(0);
        return;
    }

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


    const QVariantMap& talteen = tallennettava();
    muutettu_ = !tallennettu_.isEmpty() && (
            tallennettu_ != talteen ||
            liitteet()->tallennettaviaLiitteita() ||
            !liitteet()->liitettavat().isEmpty() );


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

    for(const auto& vienti : viennit()->viennit() ) {
        QDate pvm = vienti.pvm();

        debet += qRound64( vienti.debet() * 100.0 );
        kredit += qRound64( vienti.kredit() * 100.0 );

        if( !kp()->tilit()->tili(vienti.tili()))
            virheet |= Tosite::TILIPUUTTUU;
        if( vienti.alvKoodi() && kp()->alvIlmoitukset()->onkoIlmoitettu(pvm) && tyyppi() != TositeTyyppi::ALVLASKELMA)
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


    emit tilaTieto(muutettu_, virheet, debet / 100.0, kredit / 100.0);

}

void Tosite::nollaa(const QDate &pvm, int tyyppi)
{

    resetointiKaynnissa_ = true;
    tallennusKaynnissa_ = false;

    data_.clear();
    viennit_->asetaViennit(QVariantList());
    liitteet()->clear();
    rivit_->clear();

    asetaPvm(pvm);
    asetaTyyppi(tyyppi);

    emit ladattu();

    tallennettu_ = tallennettava();

    emit tyyppiMuuttui( this->tyyppi());
    emit pvmMuuttui( this->pvm() );
    emit otsikkoMuuttui( otsikko() );
    emit tunnisteMuuttui( tunniste() );
    emit sarjaMuuttui( sarja() );
    emit kommenttiMuuttui( kommentti());
    emit huomioMuuttui(huomio());

    resetointiKaynnissa_ = false;
    tarkasta();

}

void Tosite::tallennusValmis(QVariant *variant)
{    
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
    tallennusKaynnissa_ = false;
}

void Tosite::tallennuksessaVirhe(int virhe)
{
    emit tallennusvirhe(virhe);
    tallennusKaynnissa_ = false;
}

void Tosite::liitteetTallennettu()
{
    emit talletettu( id(), tunniste(), pvm(), sarja(), tositetila());
}

void Tosite::laitaTalteen()
{
    if (!resetointiKaynnissa_)
        tallennettu_ = tallennettava();
}

void Tosite::latausValmis()
{
    resetointiKaynnissa_ = false;
    tarkasta();
}

void Tosite::tallennaLaskuliite(QVariant *data)
{
    lataaData(data);
    QVariantMap map = tallennettava();

    LaskunTulostaja* tulostaja = new LaskunTulostaja(kp());
    connect(tulostaja, &LaskunTulostaja::laskuLiiteTallennettu, kp(),
            [this, map] { emit this->laskuTallennettu(map); }, Qt::QueuedConnection );
    tulostaja->tallennaLaskuLiite(*this);
}

QVariantMap Tosite::tallennettava() const
{

    QVariantMap map(data_);
    map.insert("viennit", viennit_->tallennettavat());

    const QVariantList& rivit = rivit_->rivit();
    if(!rivit.isEmpty())
        map.insert("rivit", rivit);

    if( !lasku_.data().isEmpty())
        map.insert("lasku", lasku_.data() );


    if( map.value("otsikko").toString().isEmpty() && viennit_->rowCount())
        map.insert("otsikko", viennit_->vienti(0).selite());

    if( tyyppi() < TositeTyyppi::MENO ||
        tyyppi() >= TositeTyyppi::SIIRTO ) {
        map.remove("kumppani");
    }

    // Jos kaikilla vienneillä on sama kumppani, liitetään se tositteeseen
    if( !map.contains("kumppani") && viennit_->rowCount() ) {
        QString knimi = viennit_->vienti(0).kumppaniNimi();
        int kid = viennit_->vienti(0).kumppaniId();

        for(int i=1; i < viennit_->rowCount(); i++) {
            QString tamanKumppani = viennit_->vienti(i).kumppaniNimi();
            int tamanKumppaniId = viennit_->vienti(i).kumppaniId();
            if( ((!tamanKumppani.isEmpty() || tyyppi() == TositeTyyppi::TILIOTE  ) && tamanKumppani != knimi) ||
                (tamanKumppaniId && tamanKumppaniId != kid))
            {                
                knimi.clear();
                kid = 0;
                break;
            }
        }
        if( !knimi.isEmpty() || kid)
            map.insert("kumppani", viennit_->vienti(0).kumppaniMap() );
    }

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
    { LISATIEDOT, "info"},
    { ALV, "alv"},
    { SARJA, "sarja"},
    { TILIOTE, "tiliote"},
    { LASKUPVM, "laskupvm"},
    { ERAPVM, "erapvm"},
    { VIITE, "viite"},
    { KIERTO, "kierto"},
    { PORTAALI, "portaali"},
    { HUOMIO, "huomio"},
    { KOMMENTTI, "kommentti"},
    { KOMMENTIT, "kommentit"},
    { TILIOTERIVI, "tilioterivi"}
};
