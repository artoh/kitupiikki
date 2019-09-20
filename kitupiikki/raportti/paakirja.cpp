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
#include "paakirja.h"

#include "db/kirjanpito.h"

#include <QDebug>

Paakirja::Paakirja(QObject *parent) : QObject(parent)
{

}

void Paakirja::kirjoita(const QDate &mista, const QDate &mihin, int optiot, int kohdennuksella,
                        int tililta)
{
    saapuneet_ = 0;
    optiot_ = optiot;

    KpKysely *saldokysely = kpk("/saldot");
    saldokysely->lisaaAttribuutti("pvm",mista);
    saldokysely->lisaaAttribuutti("alkusaldot");
    if( kohdennuksella > -1)
        saldokysely->lisaaAttribuutti("kohdennus", kohdennuksella);
    if( tililta )
        saldokysely->lisaaAttribuutti("tili", tililta);

    connect( saldokysely, &KpKysely::vastaus, this, &Paakirja::saldotSaapuu);


    KpKysely *vientikysely = kpk("/viennit");
    vientikysely->lisaaAttribuutti("alkupvm", mista);
    vientikysely->lisaaAttribuutti("loppupvm", mihin);
    vientikysely->lisaaAttribuutti("jarjestys","tili");

    if( kohdennuksella > -1)
        vientikysely->lisaaAttribuutti("kohdennus", kohdennuksella);
    if( tililta )
        vientikysely->lisaaAttribuutti("tili", tililta);

    connect( vientikysely, &KpKysely::vastaus, this, &Paakirja::viennitSaapuu);


    if( kohdennuksella > -1 )
        // Tulostetaan vain yhdestä kohdennuksesta
        rk.asetaOtsikko( tr("PÄÄKIRJAN OTE \n%1").arg( kp()->kohdennukset()->kohdennus(kohdennuksella).nimi()) ) ;
    else if( tililta)
        rk.asetaOtsikko( tr("PÄÄKIRJAN OTE"));
    else
        rk.asetaOtsikko( "PÄÄKIRJA");

    rk.asetaKausiteksti(QString("%1 - %2").arg( mista.toString("dd.MM.yyyy") )
                                             .arg( mihin.toString("dd.MM.yyyy") ) );

    rk.lisaaPvmSarake();        // Pvm
    rk.lisaaSarake("ABC1234/99 "); // Tosite
    rk.lisaaVenyvaSarake();     // Selite
    if( optiot & TulostaKohdennukset)
        rk.lisaaSarake("Kohdennusnimi"); // Kohdennus
    rk.lisaaEurosarake();   // Debet
    rk.lisaaEurosarake();   // Kredit
    rk.lisaaEurosarake();   // Saldo

    RaporttiRivi otsikko;
    otsikko.lisaa("Pvm");
    otsikko.lisaa("Tosite");
    otsikko.lisaa("Selite");
    if( optiot & TulostaKohdennukset )
        otsikko.lisaa("Kohdennus");
    otsikko.lisaa("Debet €",1,true);
    otsikko.lisaa("Kredit €",1,true);
    otsikko.lisaa("Saldo €",1, true);
    rk.lisaaOtsake(otsikko);

    saldokysely->kysy();
    vientikysely->kysy();

}

void Paakirja::saldotSaapuu(QVariant *data)
{
    saldot_ = data->toMap();
    if( ++saapuneet_ > 1)
        kirjoitaDatasta();
}

void Paakirja::viennitSaapuu(QVariant *data)
{
    viennit_ = data->toList();
    if( ++saapuneet_ > 1)
        kirjoitaDatasta();
}

void Paakirja::kirjoitaDatasta()
{

    QStringList saldotilit = saldot_.keys();

    for(auto vienti : viennit_) {
        QVariantMap map = vienti.toMap();
        int tili = map.value("tili").toInt();

        while( !saldotilit.isEmpty() && QString::number(tili) <= saldotilit.first())
        {
            aloitaTili( saldotilit.takeFirst().toInt()  );
        }

        if( tili != nykytili_.numero()) {
            aloitaTili( tili);
        }

        kirjoitaVienti( map );
    }

    while( !saldotilit.isEmpty() )
    {
        aloitaTili(saldotilit.takeFirst().toInt());
    }

    aloitaTili(); // Jotta tulee viimeisteltyä

    if( optiot_ & TulostaSummat ) {
        RaporttiRivi summa;
        summa.viivaYlle();
        summa.lihavoi();
        summa.lisaa(tr("Yhteensä"),3);

        summa.lisaa(kaikkiDebet_);
        summa.lisaa(kaikkiKredit_);
        summa.lisaa("");

        rk.lisaaRivi(summa);
    }

    emit valmis(rk);
}

void Paakirja::aloitaTili(int tilinumero)
{
    if( (debetSumma_ || kreditSumma_) && optiot_ & TulostaSummat  ) {
        RaporttiRivi summa;
        summa.viivaYlle();
        summa.lihavoi();
        summa.lisaa("",2);

        if( optiot_ & TulostaKohdennukset)
            summa.lisaa("");

        qlonglong muutos = nykytili_.onko(TiliLaji::VASTAAVAA) ?
                debetSumma_ - kreditSumma_ : kreditSumma_ - debetSumma_;
        summa.lisaa(muutos,false, true);

        summa.lisaa(debetSumma_);
        summa.lisaa(kreditSumma_);
        summa.lisaa(saldo_);

        rk.lisaaRivi(summa);

        kaikkiDebet_ += debetSumma_;
        kaikkiKredit_ += kreditSumma_;
    }

    if ( debetSumma_ || kreditSumma_ || saldo_)
        rk.lisaaRivi();     // Tyhjä rivi välille


    nykytili_ = kp()->tilit()->tiliNumerolla(tilinumero);
    if( nykytili_.onkoValidi())
    {

        RaporttiRivi rivi;
        rivi.lihavoi();
        rivi.lisaaLinkilla( RaporttiRiviSarake::TILI_NRO, nykytili_.numero(),
                            nykytili_.nimiNumero(), 5);

        saldo_ = qRound( saldot_.value( QString::number(nykytili_.numero()) ).toDouble() *100);
        rivi.lisaa( saldo_ );
        rk.lisaaRivi(rivi);

    }

    debetSumma_ = 0l;
    kreditSumma_ = 0l;

}

void Paakirja::kirjoitaVienti(QVariantMap map)
{
    RaporttiRivi rr;
    rr.lisaa( map.value("pvm").toDate() );
    rr.lisaa( map.value("tosite").toMap().value("id").toString());
    rr.lisaa( map.value("selite").toString());

    if( optiot_ & TulostaKohdennukset)
        rr.lisaa(kp()->kohdennukset()->kohdennus( map.value("kohdennus").toInt() ).nimi() );

    rr.lisaa( qRound( map.value("debet").toDouble() * 100 ));
    rr.lisaa( qRound( map.value("kredit").toDouble() * 100 ));

    debetSumma_ += qRound( map.value("debet").toDouble() * 100 );
    kreditSumma_ += qRound( map.value("kredit").toDouble() * 100 );

    if( nykytili_.onko(TiliLaji::VASTAAVAA))
    {
        saldo_ += qRound( map.value("debet").toDouble() * 100 );
        saldo_ -= qRound( map.value("kredit").toDouble() * 100 );
    } else {
        saldo_ -= qRound( map.value("debet").toDouble() * 100 );
        saldo_ += qRound( map.value("kredit").toDouble() * 100 );
    }

    rr.lisaa( saldo_);
    rk.lisaaRivi(rr);
}
