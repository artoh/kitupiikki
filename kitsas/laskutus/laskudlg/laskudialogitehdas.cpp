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
#include "laskudialogitehdas.h"

#include "model/tosite.h"
#include "kantalaskudialogi.h"
#include "tavallinenlaskudialogi.h"
#include "hyvityslaskudialogi.h"
#include "ryhmalaskudialogi.h"
#include "maksumuistusdialogi.h"

#include "db/tositetyyppimodel.h"
#include "db/kitsasinterface.h"
#include "kieli/kielet.h"
#include "db/asetusmodel.h"
#include "model/tositerivi.h"
#include "model/tositerivit.h"
#include "model/tositeviennit.h"
#include "model/tositevienti.h"
#include "model/tositeloki.h"

LaskuDialogiTehdas::LaskuDialogiTehdas(KitsasInterface *kitsas, QObject *parent) :
    QObject(parent),
    kitsas_(kitsas)
{

}

void LaskuDialogiTehdas::kaynnista(KitsasInterface *interface, QObject* parent)
{
    instanssi__ = new LaskuDialogiTehdas(interface, parent);
}

void LaskuDialogiTehdas::naytaLasku(int tositeId)
{
    Tosite* tosite = new Tosite(instanssi__);
    connect( tosite, &Tosite::ladattu, instanssi__, &LaskuDialogiTehdas::tositeLadattu);
    tosite->lataa(tositeId);
}

KantaLaskuDialogi *LaskuDialogiTehdas::myyntilasku(int asiakasId)
{
    Tosite* tosite = new Tosite();
    tosite->asetaTyyppi(TositeTyyppi::MYYNTILASKU);
    tosite->asetaKumppani(asiakasId);

    tosite->asetaLaskupvm( paivamaara() );
    tosite->lasku().setToimituspvm( paivamaara() );
    tosite->lasku().setKieli( Kielet::instanssi()->nykyinen().toUpper());
    tosite->asetaErapvm( Lasku::oikaiseErapaiva( paivamaara().addDays( instanssi__->kitsas_->asetukset()->luku(AsetusModel::LaskuMaksuaika) ) ) );
    tosite->lasku().setViivastyskorko( instanssi__->kitsas_->asetukset()->asetus(AsetusModel::LaskuPeruskorko).toDouble() + 7.0 );            
    tosite->lasku().setRiviTyyppi( oletusRiviTyyppi() );
    tosite->lasku().setHuomautusAika( instanssi__->kitsas_->asetukset()->luku("LaskuHuomautusaika") );

    KantaLaskuDialogi *dlg = new TavallinenLaskuDialogi(tosite);
    dlg->show();
    return dlg;
}

KantaLaskuDialogi *LaskuDialogiTehdas::ryhmalasku()
{
    Tosite* tosite = new Tosite();
    tosite->asetaTyyppi(TositeTyyppi::MYYNTILASKU);
    tosite->asetaLaskupvm( paivamaara() );
    tosite->asetaErapvm( paivamaara().addDays( instanssi__->kitsas_->asetukset()->luku(AsetusModel::LaskuMaksuaika) ) );
    tosite->lasku().setViivastyskorko( instanssi__->kitsas_->asetukset()->asetus(AsetusModel::LaskuPeruskorko).toDouble() + 7.0 );
    tosite->lasku().setRiviTyyppi( oletusRiviTyyppi() );

    RyhmaLaskuDialogi *dlg = new RyhmaLaskuDialogi(tosite);
    dlg->show();
    return dlg;
}

void LaskuDialogiTehdas::hyvityslasku(int hyvitettavaTositeId)
{
    Tosite* tosite = new Tosite(instanssi__);
    connect( tosite, &Tosite::ladattu, instanssi__, &LaskuDialogiTehdas::hyvitettavaLadattu);
    tosite->lataa(hyvitettavaTositeId);
}

void LaskuDialogiTehdas::kopioi(int tositeId)
{
    Tosite* tosite = new Tosite(instanssi__);    
    connect( tosite, &Tosite::ladattu, instanssi__, &LaskuDialogiTehdas::ladattuKopioitavaksi);
    tosite->lataa(tositeId);
}

void LaskuDialogiTehdas::naytaDialogi(Tosite *tosite)
{
    KantaLaskuDialogi* dlg = nullptr;

    switch (tosite->tyyppi()) {
    case TositeTyyppi::HYVITYSLASKU:
        dlg = new HyvitysLaskuDialogi(tosite);
        break;
    case TositeTyyppi::MAKSUMUISTUTUS:
        dlg = new MaksumuistusDialogi(tosite);
        break;
    default:
        dlg = new TavallinenLaskuDialogi(tosite);
    }

    if( dlg )
        dlg->show();
}

void LaskuDialogiTehdas::tositeLadattu()
{
    Tosite* tosite = qobject_cast<Tosite*>(sender());
    disconnect(tosite, &Tosite::ladattu, nullptr, nullptr);

    naytaDialogi( tosite );
}

void LaskuDialogiTehdas::hyvitettavaLadattu()
{
    Tosite* hyvitettava = qobject_cast<Tosite*>(sender());
    disconnect(hyvitettava, &Tosite::ladattu, nullptr, nullptr);

    const Lasku& hyvitettavaLasku = hyvitettava->constLasku();

    Tosite* uusi = new Tosite(instanssi__);
    uusi->asetaTyyppi( TositeTyyppi::HYVITYSLASKU );

    uusi->lasku().setKieli( hyvitettavaLasku.kieli() );
    uusi->lasku().setLahetystapa( hyvitettavaLasku.lahetystapa() );
    uusi->lasku().setAlkuperaisNumero( hyvitettavaLasku.numero().toLongLong() );
    uusi->lasku().setOsoite( hyvitettavaLasku.osoite() );
    uusi->lasku().setEmail( hyvitettavaLasku.email() );
    uusi->lasku().setAlkuperaisPvm( hyvitettavaLasku.laskunpaiva() );
    uusi->asetaViite( hyvitettavaLasku.viite() );
    uusi->lasku().setViite( hyvitettavaLasku.viite() );
    uusi->asetaKumppani( hyvitettava->kumppani() );
    uusi->lasku().setRiviTyyppi( hyvitettavaLasku.riviTyyppi() );

    TositeRivit* rivit = hyvitettava->rivit();
    for(int r=0; r < rivit->rowCount(); r++) {
        TositeRivi rivi = rivit->rivi(r);
        const QString& kpl = rivi.laskutetaanKpl();
        rivi.setLaskutetaanKpl( kpl.startsWith("-") ? kpl.mid(1) : "-" + kpl );
        rivi.setMyyntiKpl( 0 - rivi.myyntiKpl() );        
        uusi->rivit()->lisaaRivi(rivi);
    }

    HyvitysLaskuDialogi* dlg = new HyvitysLaskuDialogi(uusi);
    dlg->asetaEra( hyvitettava->viennit()->vienti(0).eraId() );
    dlg->show();

    hyvitettava->deleteLater();
}

void LaskuDialogiTehdas::ladattuKopioitavaksi()
{
    Tosite* tosite = qobject_cast<Tosite*>(sender());
    disconnect(tosite, &Tosite::ladattu, nullptr, nullptr);

    tosite->setData(Tosite::ID, 0);
    tosite->asetaTila(Tosite::POISTETTU);

    tosite->lasku().setNumero(QString());
    tosite->loki()->lataa(QVariant());  // Tyhjennetään myös loki

    QDate jaksopaiva = tosite->lasku().jaksopvm();
    int jaksonpituus = jaksopaiva.isValid() ? tosite->laskupvm().daysTo(jaksopaiva) : 0;
    int maksuaika = tosite->lasku().laskunpaiva().daysTo( tosite->lasku().erapvm() );

    tosite->asetaLaskupvm( paivamaara() );
    tosite->asetaErapvm( paivamaara().addDays( maksuaika ) );

    tosite->lasku().setLaskunpaiva( tosite->laskupvm());
    tosite->lasku().setErapaiva( tosite->erapvm() );

    tosite->lasku().setToimituspvm( tosite->laskupvm() );

    // Irroitetaan kopioi samasta tase-erästä, jotta saa oman seurantansa
    TositeVienti vienti = tosite->viennit()->vienti(0);
    if( vienti.eraId())
        vienti.setEra(-1);
    tosite->viennit()->asetaVienti(0, vienti);


    if( jaksonpituus) {
        tosite->lasku().setJaksopvm( tosite->laskupvm().addDays( jaksonpituus ) );
    }


    ViiteNumero viite( tosite->viite());
    if( viite.tyyppi() != ViiteNumero::ASIAKAS && viite.tyyppi() != ViiteNumero::HUONEISTO) {
        tosite->asetaViite(QString());
        tosite->lasku().setViite(ViiteNumero(0));
    }   

    tosite->setData(Tosite::TUNNISTE, 0);
    naytaDialogi( tosite );
}

Lasku::Rivityyppi LaskuDialogiTehdas::oletusRiviTyyppi()
{
    if( !instanssi__->kitsas_->asetukset()->onko(AsetusModel::AlvVelvollinen))
        return Lasku::BRUTTORIVIT;

    int tyyppi = instanssi__->kitsas_->asetukset()->luku(AsetusModel::LaskuRiviTyyppi);
    if( tyyppi == Lasku::BRUTTORIVIT)
        return Lasku::BRUTTORIVIT;
    else if( tyyppi == Lasku::PITKATRIVIT)
        return Lasku::PITKATRIVIT;
    else
        return Lasku::NETTORIVIT;
}

QDate LaskuDialogiTehdas::paivamaara()
{
    return instanssi__->kitsas_ ? instanssi__->kitsas_->paivamaara() : QDate::currentDate();
}

LaskuDialogiTehdas* LaskuDialogiTehdas::instanssi__ = nullptr;
