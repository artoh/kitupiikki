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
#include "maksumuistutusmuodostaja.h"

#include "model/tosite.h"
#include "model/tositevienti.h"
#include "model/tositeviennit.h"
#include "model/tositerivi.h"
#include "model/tositerivit.h"

#include "db/kitsasinterface.h"
#include "db/asetusmodel.h"
#include "db/tositetyyppimodel.h"

#include "kieli/monikielinen.h"

#include <QDebug>

MaksumuistutusMuodostaja::MaksumuistutusMuodostaja(KitsasInterface *kitsas)
    : kitsas_(kitsas)
{

}

void MaksumuistutusMuodostaja::muodostaMuistutukset(Tosite *tosite, const QDate &pvm,
                                                    int eraId, Euro maksumuistus,
                                                    Euro korkoSaldo,
                                                    const QDate &korkoAlkaa, const QDate &korkoLoppuu,
                                                    double korko, int vastatili)
{
    qDebug() << " Korko "  << korkoAlkaa.toString() << " - " << korkoLoppuu.toString() << " " << korko << " %";


    tosite->rivit()->lataa(QVariantList());
    tosite->viennit()->tyhjenna();
    aiempiSaldo(tosite, korkoSaldo);

    Euro yhteensa = maksumuistus + laskeKorko( korkoSaldo, korkoAlkaa, korkoLoppuu, korko );
    vastakirjaus(tosite, yhteensa, eraId, pvm, vastatili ? vastatili : kitsas_->asetukset()->luku("LaskuSaatavatili") );

    muistutusMaksu(tosite, maksumuistus, pvm);
    kirjaaKorko(tosite, korkoSaldo, korkoAlkaa, korkoLoppuu, korko, pvm);

    tosite->lasku().setSumma( korkoSaldo + yhteensa );
    tosite->lasku().setAiempiSaldo( korkoSaldo );
}

Euro MaksumuistutusMuodostaja::laskeKorko(Euro korkoSaldo, const QDate &korkoAlkaa, const QDate &korkoLoppuu, double korko)
{
    double paivakorko = paivaKorko(korkoAlkaa, korkoLoppuu, korko, korkoSaldo);
    qlonglong paivat = korkoAlkaa.daysTo(korkoLoppuu);
    return Euro::fromDouble( paivat * paivakorko );
}

void MaksumuistutusMuodostaja::aiempiSaldo(Tosite *tosite, Euro aiempiSaldo)
{
    if(aiempiSaldo.cents()) {
        TositeRivi aiempi;
        aiempi.setNimike( kitsas_->kaanna("mmrivi", tosite->lasku().kieli()) );
        aiempi.setANetto(0.0);       

        aiempi.setBruttoYhteensa( aiempiSaldo );
        tosite->rivit()->lisaaRivi(aiempi);
    }
}

void MaksumuistutusMuodostaja::muistutusMaksu(Tosite *tosite, Euro maksu, const QDate& pvm)
{
    if( maksu.cents()) {
        TositeVienti mmvienti;
        mmvienti.setPvm(pvm);
        mmvienti.setTili(kitsas_->asetukset()->luku("LaskuMaksumuistutustili",9170)); // Tämä asetuksiin
        mmvienti.setTyyppi(TositeTyyppi::TULO + TositeVienti::KIRJAUS);
        mmvienti.setKredit(maksu);
        mmvienti.setKumppani(tosite->kumppani());
        tosite->viennit()->lisaa(mmvienti);

        TositeRivi rivi;
        rivi.setNimike(kitsas_->kaanna("muistutusmaksu", tosite->lasku().kieli()));
        rivi.setMyyntiKpl(1.0);
        rivi.setLaskutetaanKpl("1");
        rivi.setANetto( maksu.toDouble() );
        rivi.setTili( kitsas_->asetukset()->luku("LaskuMaksumuistutustili",9170));        
        tosite->rivit()->lisaaRivi(rivi);

        tosite->lasku().setMuistutusmaksu( maksu );
    } else {
        tosite->lasku().setMuistutusmaksu(0);
    }

}

void MaksumuistutusMuodostaja::kirjaaKorko(Tosite *tosite, Euro korkosaldo, const QDate &alkupvm, const QDate &loppupvm, double korko, const QDate &pvm)
{
    double paivakorko = paivaKorko(alkupvm, loppupvm, korko, korkosaldo);
    qlonglong paivat = alkupvm.daysTo(loppupvm);
    Euro yhteensa = Euro::fromDouble( paivakorko * paivat );
    const QString& kieli = tosite->lasku().kieli().toLower();

    if( yhteensa.cents() ) {
        QString selite = kitsas_->kaanna("viivkorko", kieli) +
                QString(" %1 - %2")
                .arg(alkupvm.toString("dd.MM.yyyy"), loppupvm.toString("dd.MM.yyyy"));

        TositeVienti korkovienti;
        korkovienti.setPvm( pvm );
        korkovienti.setTili( kitsas_->asetukset()->luku("LaskuViivastyskorkotili",9170));
        korkovienti.setTyyppi(TositeTyyppi::TULO + TositeVienti::KIRJAUS);
        korkovienti.setKredit(yhteensa);
        korkovienti.setSelite(selite);
        korkovienti.setKumppani(tosite->kumppani());

        tosite->viennit()->lisaa(korkovienti);

        TositeRivi rivi;
        rivi.setNimike(selite);
        rivi.setMyyntiKpl( paivat );
        rivi.setLaskutetaanKpl( QString::number(paivat) );
        rivi.setUNkoodi("DAY");
        rivi.setANetto( paivakorko );
        rivi.setBruttoYhteensa( yhteensa );
        rivi.setTili(kitsas_->asetukset()->luku("LaskuViivastyskorkotili",9170));

        tosite->rivit()->lisaaRivi(rivi);
    }

    tosite->lasku().setKorkoAlkaa(alkupvm);
    tosite->lasku().setKorkoLoppuu(loppupvm);
    tosite->lasku().setKorko(yhteensa);
    tosite->lasku().setViivastyskorko(korko);    

}

double MaksumuistutusMuodostaja::paivaKorko(const QDate &alkupvm, const QDate &loppupvm, double korko, Euro peruste)
{
    if( !alkupvm.isValid() || !loppupvm.isValid() || korko < 1e-5 || !peruste.cents() || loppupvm <= alkupvm)
        return 0.0;

    return peruste.toDouble() * korko  / ( 100.0 * alkupvm.daysInYear() );
}

void MaksumuistutusMuodostaja::vastakirjaus(Tosite* tosite, const Euro &euro, int era, const QDate& pvm, int vastatili)
{
    TositeVienti vienti;
    vienti.setEra(era);
    vienti.setPvm(pvm);
    vienti.setTili( vastatili );
    vienti.setTyyppi(TositeTyyppi::TULO + TositeVienti::VASTAKIRJAUS);
    vienti.setKumppani(tosite->kumppani());
    vienti.setSelite(tosite->otsikko());
    vienti.setDebet(euro);
    tosite->viennit()->lisaa(vienti);
}
