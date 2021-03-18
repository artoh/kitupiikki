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

#include "kitsas.h"

#include "model/lasku.h"
#include "model/tositerivit.h"
#include "model/tositerivi.h"
#include "db/kitsasinterface.h"
#include "db/asetusmodel.h"

#include <QPainter>

LaskuRuudukonTayttaja::LaskuRuudukonTayttaja(KitsasInterface *kitsas) :
    kitsas_(kitsas)
{

}

TulostusRuudukko LaskuRuudukonTayttaja::tayta(Tosite &tosite)
{
    kieli_ = tosite.lasku().kieli().toLower();
    bruttolaskenta_ = tosite.lasku().bruttoVerolaskenta();

    alv_.yhdistaRiveihin( tosite.rivit());
    alv_.asetaBruttoPeruste( bruttolaskenta_ );
    alv_.paivita();

    if( tosite.rivit()->rowCount() == 0)
        return TulostusRuudukko();

    tutkiSarakkeet(tosite);
    kirjoitaSarakkeet();
    taytaSarakkeet(tosite);
    taytaSummat();
    return ruudukko_;
}

TulostusRuudukko LaskuRuudukonTayttaja::alvRuudukko(QPainter *painter)
{
    painter->save();
    painter->setFont(QFont("FreeSans", 10));
    qreal vahintaan = painter->fontMetrics().horizontalAdvance("1 000,00 e");

    painter->restore();

    // Verottomalle ei tulosteta myöskään alv-erittelyä
    if( alv_.veroton() )
        return TulostusRuudukko();

    bool vainSumma = !alv_.vero().cents();

    TulostusRuudukko veroruudukko;
    veroruudukko.lisaaSarake("");

    if( vainSumma ) {
        veroruudukko.lisaaSarake( "", Qt::AlignRight, vahintaan );
    } else {
        veroruudukko.lisaaSarake( kitsas_->kaanna("netto", kieli_), Qt::AlignRight, vahintaan );
        veroruudukko.lisaaSarake( kitsas_->kaanna("vero", kieli_), Qt::AlignRight, vahintaan );
        veroruudukko.lisaaSarake( kitsas_->kaanna("brutto", kieli_), Qt::AlignRight, vahintaan );
    }

    QList<int> indeksit = alv_.indeksitKaytossa();
    for(int indeksi : indeksit) {
        int verokoodi = alv_.alvkoodi(indeksi);
        QStringList tekstit;

        if( verokoodi == AlvKoodi::MYYNNIT_NETTO) {
            tekstit << QString("%1 %L2 %").arg(kitsas_->kaanna("alv", kieli_))
                                        .arg(alv_.veroprosentti(indeksi),0,'f',2);
        } else {
            tekstit << kitsas_->kaanna( QString("alv%1").arg(verokoodi), kieli_ );
        }

        tekstit << alv_.netto(indeksi).display();
        if( !vainSumma ) {
            tekstit << alv_.vero(indeksi).display(false);
            tekstit << alv_.brutto(indeksi).display();
        }
        veroruudukko.lisaaRivi(tekstit);
    }

    if( indeksit.count() > 1) {
        QStringList stekstit;
        stekstit << kitsas_->kaanna("Yhteensa", kieli_ );
        stekstit << alv_.netto().display();
        if( !vainSumma) {
            stekstit << alv_.vero().display();
            stekstit << alv_.brutto().display();
        }
        veroruudukko.lisaaRivi(stekstit, true);
    }

    return veroruudukko;
}

TulostusRuudukko LaskuRuudukonTayttaja::kuukausiRuudukko(const Lasku &lasku, QPainter *painter)
{
    QDate alkaa = lasku.toimituspvm();
    QDate paattyy = lasku.jaksopvm();
    int erapaiva = lasku.toistuvanErapaiva();

    if( !alkaa.isValid() || !paattyy.isValid() || !erapaiva || paattyy < alkaa)
        return TulostusRuudukko();

    painter->setFont(QFont("FreeSans", 10));
    qreal euroleveys = painter->fontMetrics().horizontalAdvance("10 000,00 e");

    TulostusRuudukko toistot;
    toistot.lisaaSarake( kitsas_->kaanna("erapvm", kieli_) );
    toistot.lisaaSarake( kitsas_->kaanna("viitenro", kieli_) );
    toistot.lisaaSarake( kitsas_->kaanna("maksettavaa", kieli_), Qt::AlignRight, euroleveys );

    if( alkaa.day() > erapaiva)
        alkaa = alkaa.addMonths(1);

    while( alkaa <= paattyy) {
        QStringList tekstit;
        tekstit << QString("%1.%2.%3").arg(erapaiva, 2, 10, QChar('0'))
                                      .arg(alkaa.month(), 2, 10, QChar('0'))
                                      .arg(alkaa.year());
        tekstit << ( kitsas_->asetukset()->onko(AsetusModel::LaskuRF) ?
                       lasku.viite().rfviite() : lasku.viite().valeilla() );
        tekstit << alv_.brutto().display();
        toistot.lisaaRivi(tekstit);
        alkaa = alkaa.addMonths(1);
    }
    return toistot;

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
        if( bruttolaskenta_ ) {
            tekstit << rivi.bruttoYhteensa().display();
        } else {
            tekstit << Euro::fromDouble( rivi.nettoYhteensa() ).display();
        }

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

void LaskuRuudukonTayttaja::taytaSummat()
{
    if( alv_.vero().cents() && !bruttolaskenta_ ) {
        ruudukko_.lisaaSummaRivi( kitsas_->kaanna("YhteensaVeroton", kieli_), alv_.netto().display() );
        ruudukko_.lisaaSummaRivi( kitsas_->kaanna("Vero", kieli_), alv_.vero().display() );
        ruudukko_.lisaaSummaRivi( kitsas_->kaanna("YhteensaVerollinen", kieli_), alv_.brutto().display());
    } else {
        ruudukko_.lisaaSummaRivi( kitsas_->kaanna("Yhteensa", kieli_), alv_.brutto().display() );
    }
}

