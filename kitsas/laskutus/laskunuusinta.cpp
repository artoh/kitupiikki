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
#include "laskunuusinta.h"

#include "db/kirjanpito.h"
#include "pilvi/pilvimodel.h"

#include "model/tosite.h"
#include "model/tositerivi.h"
#include "model/tositerivit.h"
#include "laskutus/tuotemodel.h"
#include "laskutus/laskudlg/rivivientigeneroija.h"
#include "rekisteri/maamodel.h"
#include <QMessageBox>

LaskunUusinta::LaskunUusinta(QObject *parent) : QObject(parent),
    tosite_{new Tosite(this)}
{    
    connect( &timer_, &QTimer::timeout, this, &LaskunUusinta::uusiLaskut);
    connect( qobject_cast<Kirjanpito*>(parent), &Kirjanpito::tietokantaVaihtui, this, &LaskunUusinta::ajastaUusinta);
    connect( tosite_, &Tosite::ladattu, this, &LaskunUusinta::uusittavaLadattu);

}

void LaskunUusinta::ajastaUusinta()
{
    timer_.start(3000);
}

void LaskunUusinta::uusiLaskut()
{
    timer_.stop();

    if( qobject_cast<PilviModel*>(kp()->yhteysModel()) &&
        kp()->yhteysModel()->onkoOikeutta( YhteysModel::LASKU_LAATIMINEN ) && !busy_) {
        KpKysely *kysely = kpk("/myyntilaskut");
        kysely->lisaaAttribuutti("uusittavat", kp()->paivamaara());
        connect( kysely, &KpKysely::vastaus, this, &LaskunUusinta::listaSaapuu);
        kysely->kysy();
    }
}

void LaskunUusinta::listaSaapuu(QVariant *lista)
{
    QVariantList laskut = lista->toList();
    qDebug() << laskut.count() << " uusittavaa laskua ";

    if( laskut.count() &&
        (!kp()->tilikaudet()->onkoTilikautta( kp()->paivamaara())
            || kp()->tilitpaatetty() >= kp()->paivamaara() ) ) {
        QMessageBox::warning(nullptr, tr("Laskuja ei voitu uusia"),
            tr("Toistuvia laskuja ei voitu uusia, koska nykyiselle päivälle ei ole avointa tilikautta.\n"
               "Avaa tilikausi ja käynnistä ohjelma uudelleen, jotta laskut voidaan uusia."));
        return;
    }


    for(const auto& item : qAsConst( laskut )) {
        jono_.enqueue( item.toMap().value("id").toInt() );
    }
    if( !jono_.isEmpty() && !busy_) {
        emit kp()->onni(tr("Luodaan toistuvia laskuja"), Kirjanpito::Haetaan);
        busy_ = true;
        uusiSeuraava();
    }
}

void LaskunUusinta::uusiSeuraava()
{

    if( jono_.isEmpty()) {
        emit kp()->onni(tr("Toistuvat laskut luotu Lähtevät-välilehdelle."));
        emit kp()->kirjanpitoaMuokattu();
        busy_ = false;
    } else {
        int id = jono_.dequeue();
        tosite_->lataa(id);
    }
}

void LaskunUusinta::uusittavaLadattu()
{
    qDebug() << " Uusittava tosite " << tosite_->id() << " ladattu ";
    if( !tosite_->id() || !busy_) return;

    uusi_ = new Tosite(this);
    uusi_->lataa( tosite_->tallennettava() );
    const Lasku& lasku = tosite_->constLasku();

    uusi_->setData(Tosite::ID, 0);
    uusi_->lasku().setNumero(QString());

    if( lasku.valvonta() != Lasku::ASIAKAS && lasku.valvonta() != Lasku::VAKIOVIITE && lasku.valvonta() != Lasku::HUONEISTO) {
        uusi_->asetaViite(QString());
        uusi_->lasku().setViite(QString());
    }

    uusi_->asetaPvm( kp()->paivamaara() );
    uusi_->asetaLaskupvm( kp()->paivamaara() );
    uusi_->lasku().setLaskunpaiva( kp()->paivamaara() );
    uusi_->lasku().setToimituspvm( lasku.jaksopvm().addDays(1) );
    QDate jaksonpaiva = lasku.jaksopvm().addMonths( lasku.toistoJaksoPituus() );
    if( lasku.jaksopvm().day() == lasku.jaksopvm().daysInMonth()) {
        // #1206 Tasakuukaudet kun jakso kuukauden loppuun (riippumatta kuukauden pituudesta)
        jaksonpaiva.setDate( jaksonpaiva.year(), jaksonpaiva.month(), jaksonpaiva.daysInMonth() );
    }
    uusi_->lasku().setJaksopvm( jaksonpaiva );

    if( uusi_->lasku().jaksopvm() <= lasku.toistoLoppuu() ||
            !lasku.toistoLoppuu().isValid() ) {
        const QDate vanhaToistoPvm = lasku.toistoPvm();
        const int paivaa = lasku.jaksopvm().daysTo(vanhaToistoPvm);
        const QDate uusiToisto = uusi_->lasku().jaksopvm().addDays( paivaa );
        uusi_->lasku().setToisto( uusiToisto, lasku.toistoJaksoPituus(),
                                  lasku.toistoHinnastonMukaan(), lasku.toistoLoppuu());
    } else {
        uusi_->lasku().lopetaToisto();
    }

    uusi_->lasku().setViivastyskorko( kp()->asetukset()->asetus(AsetusModel::LaskuPeruskorko).toDouble() + 7.0 );
    if( lasku.erapvm().isValid()) {
        QDate erapvm = Lasku::oikaiseErapaiva( kp()->paivamaara().addDays( lasku.laskunpaiva().daysTo(lasku.erapvm()) ) );
        uusi_->asetaErapvm(erapvm);
        uusi_->lasku().setErapaiva( erapvm );
    }

    paivitaYleinenAlv( uusi_->lasku().jaksopvm() );
    paivitaHinnat();

    RiviVientiGeneroija riviGeneroija(kp());
    riviGeneroija.generoiViennit(uusi_, Kitsas::UUSI_ERA);

    if( tosite_->kumppani() ) {
        KpKysely* asiakasHaku = kpk(QString("/kumppanit/%1").arg(tosite_->kumppani()));
        connect( asiakasHaku, &KpKysely::vastaus, this, &LaskunUusinta::asiakasSaapuu);
        connect( asiakasHaku, &KpKysely::virhe, this, &LaskunUusinta::jatkaTallentamaan);
        asiakasHaku->kysy();
    } else {
        jatkaTallentamaan();
    }
}

void LaskunUusinta::paivitaHinnat()
{
    for(int i=0; i < uusi_->rivit()->rowCount(); i++) {
        const TositeRivi& rivi = uusi_->rivit()->rivi(i);
        if( !rivi.tuote())
            continue;

        const Tuote& tuote = kp()->tuotteet()->tuote( rivi.tuote() );
        if( qAbs(tuote.ahinta()) > 1e-5)
            uusi_->rivit()->setData( tosite_->rivit()->index(i, TositeRivit::AHINTA),
                                       tuote.ahinta());
    }
}

void LaskunUusinta::paivitaYleinenAlv(const QDate &pvm)
{
    for(int i=0; i < uusi_->rivit()->rowCount(); i++) {
        const double prossa = uusi_->rivit()->rivi(i).alvProsentti();
        if( prossa == 24.0) {
            uusi_->rivit()->setData(  uusi_->rivit()->index(i, TositeRivit::ALV),
                                      yleinenAlv(pvm) / 100.0,
                                      TositeRivit::AlvProsenttiRooli);
        }
        // Toistuvassa laskussa jos alv-velvollisuus on lakannut, niin riviä
        // ei saa luoda arvonlisäverollisena
        if( !kp()->onkoAlvVelvollinen(pvm) && uusi_->rivit()->rivi(i).alvkoodi() != AlvKoodi::EIALV) {
            const double brutto = uusi_->rivit()->rivi(i).bruttoYhteensa().toDouble();
            uusi_->rivit()->setData(  uusi_->rivit()->index(i, TositeRivit::ALV),
                                    0,
                                    TositeRivit::AlvProsenttiRooli);
            uusi_->rivit()->setData(  uusi_->rivit()->index(i, TositeRivit::ALV),
                                    AlvKoodi::EIALV,
                                    TositeRivit::AlvKoodiRooli);
            uusi_->rivit()->setData(  uusi_->rivit()->index(i, TositeRivit::YHTEENSA),
                                    brutto,
                                    Qt::EditRole);


        }
    }
}

void LaskunUusinta::asiakasSaapuu(QVariant *asiakas)
{
    // Päivitetään asiakkaan yhteystiedot
    QVariantMap map = asiakas->toMap();
    uusi_->lasku().setOsoite( MaaModel::instanssi()->muotoiltuOsoite(map) );
    uusi_->lasku().setEmail( map.value("email").toString());
    jatkaTallentamaan();
}

void LaskunUusinta::jatkaTallentamaan()
{
    connect( uusi_, &Tosite::laskuTallennettu, this, &LaskunUusinta::laskuUusittu);
    uusi_->tallennaLasku(Tosite::VALMISLASKU);
}

void LaskunUusinta::laskuUusittu()
{
    KpKysely *kysely = kpk(QString("/tositteet/%1").arg(tosite_->id()), KpKysely::PATCH);
    QVariantMap map;
    if( uusi_ ) {
        uusi_->deleteLater();
        uusi_ = nullptr;
    }
    map.insert("laskutoisto", QVariant());
    connect( kysely, &KpKysely::vastaus, this, &LaskunUusinta::uusiSeuraava );
    kysely->kysy(map);
}
