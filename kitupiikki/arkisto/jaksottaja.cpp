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
#include "jaksottaja.h"
#include "db/kirjanpito.h"

#include "ui_poistaja.h"

#include "model/tosite.h"
#include "model/tositeviennit.h"
#include "model/tositevienti.h"
#include "db/tositetyyppimodel.h"

#include <QMessageBox>
#include <QDebug>
#include <QJsonDocument>

Jaksottaja::Jaksottaja(QWidget *parent)
    : QDialog(parent),
      ui(new Ui::Poistaja)
{
    ui->setupUi(this);
}

Jaksottaja::~Jaksottaja()
{
    delete ui;
}

bool Jaksottaja::teeJaksotukset(const Tilikausi &kausi, const QVariantList &jaksotukset, double verovelka)
{
    qlonglong verovelkasentit = qRound64(verovelka * 100.0);

    ui->otsakeLabel->setText( tr("Vahvista tilinpäätösjaksotukset"));
    RaportinKirjoittaja selvitys = jaksotusSelvitys(kausi, jaksotukset, verovelkasentit);
    ui->browser->setHtml( selvitys.html());

    if( !selvitys.tyhja() && exec()) {
        kirjaaTilinpaatokseen( kausi.paattyy(), jaksotukset, verovelkasentit);
        return true;
    }
    return false;
}

void Jaksottaja::kirjaaTilinpaatokseen(const QDate &pvm, const QVariantList &jaksotukset, qlonglong verovelkasentit)
{
    Tosite* tosite = new Tosite(this);

    tosite->asetaPvm( pvm );
    tosite->asetaTyyppi( TositeTyyppi::JAKSOTUS );
    tosite->asetaOtsikko( tr("Tilinpäätösjaksotukset"));

    for( auto jaksotus : jaksotukset) {
        TositeVienti vienti;
        TositeVienti vasta;

        vienti.setPvm( pvm );
        vasta.setPvm( pvm );

        vienti.setTyyppi( TositeVienti::JAKSOTUS_TP + TositeVienti::KIRJAUS);
        vasta.setTyyppi( TositeVienti::JAKSOTUS_TP + TositeVienti::VASTAKIRJAUS);

        QVariantMap map = jaksotus.toMap();

        vienti.setTili( map.value("tili").toInt());

        if( map.contains("debet")) {
            vienti.setDebet( map.value("debet").toDouble());
            vasta.setKredit( map.value("debet").toDouble());
            vasta.setTili( kp()->tilit()->tiliTyypilla(TiliLaji::SIIRTOVELKA).numero() );
        } else {
            vienti.setKredit( map.value("kredit").toDouble());
            vasta.setDebet(map.value("kredit").toDouble());
            vasta.setTili( kp()->tilit()->tiliTyypilla(TiliLaji::SIIRTOSAATAVA).numero());
        }

        vasta.setEra(-1);
        vienti.setKohdennus( map.value("kohdennus").toInt());
        vasta.setKohdennus( map.value("kohdennus").toInt());

        QDate jaksoalkaa = map.value("jaksoalkaa").toDate();
        QDate jaksoloppuu = map.value("jaksoloppuu").toDate();

        if( jaksoalkaa >= pvm || jaksoloppuu > pvm)
            vasta.setJaksoalkaa(  jaksoalkaa > pvm ? jaksoalkaa : pvm.addDays(1) );

        if( jaksoloppuu > pvm)
            vasta.setJaksoloppuu( jaksoloppuu );

        QString selite = QString("%1 %2")
                .arg( kp()->tositeTunnus( map.value("tunniste").toInt(),
                                          map.value("pvm").toDate(),
                                          map.value("sarja").toString()))
                .arg( map.value("selite").toString());
        vienti.setSelite( selite );
        vasta.setSelite( selite );


        tosite->viennit()->lisaa(vasta);
        tosite->viennit()->lisaa(vienti);
    }

    if( verovelkasentit ) {
        TositeVienti vienti;
        TositeVienti vasta;

        vienti.setPvm( pvm );
        vasta.setPvm( pvm );

        vienti.setTyyppi( TositeVienti::JAKSOTUS_TP + TositeVienti::KIRJAUS);
        vasta.setTyyppi( TositeVienti::JAKSOTUS_TP + TositeVienti::VASTAKIRJAUS);

        vienti.setTili( kp()->tilit()->tiliTyypilla(TiliLaji::VEROVELKA).numero() );
        vasta.setTili( kp()->tilit()->tiliTyypilla(TiliLaji::VEROSAATAVA).numero() );

        vienti.setKredit(verovelkasentit);
        vasta.setDebet(verovelkasentit);

        QString selite = tr("Negatiivisen verovelan kirjaaminen verosaataviin");
        vienti.setSelite(selite);
        vasta.setSelite(selite);

        tosite->viennit()->lisaa(vasta);
        tosite->viennit()->lisaa(vienti);
    }


    KpKysely *kysely = kpk("/tositteet", KpKysely::POST);
    connect( kysely, &KpKysely::vastaus,
             [this, pvm] (QVariant* data) { this->kirjaaTilinavaukseen(data, pvm.addDays(1)); });
    kysely->kysy( tosite->tallennettava() );

}

RaportinKirjoittaja Jaksottaja::jaksotusSelvitys(const Tilikausi &kausi, const QVariantList &jaksotukset, qlonglong verovelkasentit)
{
    RaportinKirjoittaja rk;
    rk.asetaOtsikko(tr("TILINPÄÄTÖSJAKSOTUKSET"));
    rk.asetaKausiteksti( kausi.paattyy().toString("dd.MM.yyyy"));

    rk.lisaaPvmSarake();
    rk.lisaaSarake("TUN123456");
    rk.lisaaVenyvaSarake();
    rk.lisaaEurosarake();

    RaporttiRivi ylaotsikko;
    ylaotsikko.lisaa("Tili",2);
    rk.lisaaOtsake(ylaotsikko);

    RaporttiRivi otsikko;
    otsikko.lisaa("Pvm");
    otsikko.lisaa("Tunniste");
    otsikko.lisaa("Selite");
    otsikko.lisaa("Debet €");
    otsikko.lisaa("Kredit €");
    rk.lisaaOtsake(otsikko);        

    int edellinentili = 0;
    for( auto rivi : jaksotukset) {
        QVariantMap map = rivi.toMap();
        Tili* tili = kp()->tilit()->tili( map.value("tili").toInt());
        if( !tili)
            continue;

        if( tili->numero() != edellinentili) {
            rk.lisaaRivi();
            RaporttiRivi tilirivi;
            tilirivi.lisaa( tili->nimiNumero(), 2 );
            tilirivi.lihavoi();
            rk.lisaaRivi(tilirivi);
            edellinentili = tili->numero();
        }

        RaporttiRivi rr;
        rr.lisaa( map.value("pvm").toDate() );
        rr.lisaa( kp()->tositeTunnus( map.value("tunniste").toInt(),
                                      map.value("pvm").toDate(),
                                      map.value("sarja").toString(), true ));
        rr.lisaa( map.value("selite").toString());
        rr.lisaa( map.value("debet").toDouble());
        rr.lisaa( map.value("kredit").toDouble());
        rk.lisaaRivi(rr);
    }

    if( verovelkasentit ) {
        RaporttiRivi vrivi;
        vrivi.lisaa(tr("Verosaamiseksi kirjattava negatiivinen verovelka"),3);
        vrivi.lisaa(verovelkasentit);
        rk.lisaaRivi(vrivi);
    }

    return rk;
}

void Jaksottaja::kirjaaTilinavaukseen(QVariant *data, const QDate &pvm)
{
    Tosite paatos;
    paatos.lataaData(data);

    Tosite* avaus = new Tosite(this);
    avaus->asetaPvm( pvm );
    avaus->asetaTyyppi( TositeTyyppi::JAKSOTUS );
    avaus->asetaOtsikko( tr("Tilinavauksen jaksotuskirjaukset"));

    for(int i=0; i < paatos.viennit()->rowCount(); i++) {
        // Tilinavauksessa samat viennit kuin päätöksessä, mutta vain toisin päin

        TositeVienti vienti = paatos.viennit()->vienti(i);
        TositeVienti uusi;

        uusi.setPvm(pvm);

        if( vienti.tyyppi() == TositeVienti::JAKSOTUS_TP + TositeVienti::KIRJAUS)
            uusi.setTyyppi( TositeVienti::JAKSOTUS_TA + TositeVienti::KIRJAUS);
        else
            uusi.setTyyppi( TositeVienti::JAKSOTUS_TA + TositeVienti::VASTAKIRJAUS);

        uusi.setSelite( vienti.selite());
        uusi.setTili( vienti.tili());
        uusi.setKohdennus( vienti.kohdennus() );
        uusi.setEra( vienti.eraId());
        uusi.setJaksoalkaa( vienti.jaksoalkaa());
        uusi.setJaksoloppuu( vienti.jaksoloppuu());

        if( vienti.kredit() > 1e-5) {
            uusi.setDebet( vienti.kredit());
        } else {
            uusi.setKredit( vienti.debet());
        }

        avaus->viennit()->lisaa(uusi);
    }
    connect( avaus, &Tosite::talletettu, this, &Jaksottaja::jaksotettu);
    avaus->tallenna();
}
