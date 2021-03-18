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
#include "laskuruudukontayttaja.h"

#include "model/lasku.h"
#include "model/tositerivit.h"
#include "model/tositerivi.h"
#include "db/kitsasinterface.h"

LaskuRuudukonTayttaja::LaskuRuudukonTayttaja(KitsasInterface *kitsas) :
    kitsas_(kitsas)
{

}

TulostusRuudukko LaskuRuudukonTayttaja::tayta(Tosite &tosite)
{
    kieli_ = tosite.lasku().kieli().toLower();
    tutkiSarakkeet(tosite);
    kirjoitaSarakkeet();
    taytaSarakkeet(tosite);
    taytaSummat(tosite);
    return ruudukko_;
}

void LaskuRuudukonTayttaja::tutkiSarakkeet(Tosite &tosite)
{
    const TositeRivi& ekarivi = tosite.rivit()->rivi(0);
    int alvkoodi = ekarivi.alvkoodi();
    int alvPromille = qRound( ekarivi.aleProsentti() * 10);

    for( int i = 0; i < tosite.rivit()->rowCount(); i++) {
        const TositeRivi& rivi = tosite.rivit()->rivi(i);

        int rivinAlvkoodi = rivi.alvkoodi();
        int rivinAlvPromille = qRound( rivi.alvProsentti() * 10);

        if( rivinAlvkoodi != alvkoodi ||
            rivinAlvPromille != alvPromille ) {
            alvSarake_ = true;
        }

        if( rivi.aleProsentti() > 1e-3 || rivi.euroAlennus().cents()) {
            aleSarake_ = true;
        }
    }
}

void LaskuRuudukonTayttaja::kirjoitaSarakkeet()
{
    lisaaSarake("nimike");
    lisaaSarake("lkm",Qt::AlignRight); //Määrä
    lisaaSarake("");    // Yksikkö
    lisaaSarake("anetto", Qt::AlignRight);
    if( aleSarake_ )
        lisaaSarake("ale%", Qt::AlignRight);
    if( alvSarake_)
        lisaaSarake("alv",Qt::AlignRight);
    lisaaSarake("yhteensa", Qt::AlignRight);
}

void LaskuRuudukonTayttaja::lisaaSarake(const QString &koodi, Qt::AlignmentFlag tasaus)
{
    ruudukko_.lisaaSarake( kitsas_->kaanna(koodi, kieli_), tasaus );
}

void LaskuRuudukonTayttaja::taytaSarakkeet(Tosite &tosite)
{
    TositeRivit* rivit = tosite.rivit();
    for(int i=0; i < rivit->rowCount(); i++) {
        const TositeRivi& rivi = rivit->rivi(i);
        QStringList tekstit;

        tekstit << nimikesarake(rivi);
        tekstit << rivit->index(i, TositeRivit::MAARA).data().toString();
        tekstit << rivit->index(i, TositeRivit::YKSIKKO).data().toString();
        tekstit << rivit->index(i, TositeRivit::AHINTA).data().toString();
        if( aleSarake_)
            tekstit << rivit->index(i, TositeRivit::ALE).data().toString();
        if( alvSarake_)
            tekstit << rivit->index(i, TositeRivit::ALV).data().toString();
        tekstit << rivit->index(i, TositeRivit::BRUTTOSUMMA).data().toString();

        ruudukko_.lisaaRivi(tekstit);
    }
}

QString LaskuRuudukonTayttaja::nimikesarake(const TositeRivi &rivi)
{
    QString txt = rivi.nimike();
    // TODO Kaikki tarpeellinen ;)
    return txt;
}

void LaskuRuudukonTayttaja::taytaSummat(Tosite &tosite)
{
    ruudukko_.lisaaSummaRivi( kitsas_->kaanna("yhteensa", kieli_), tosite.rivit()->yhteensa().display() );
}

