/*
   Copyright (C) 2017 Arto Hyvättinen

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

#include <cmath>

#include "poistaja.h"
#include "ui_poistaja.h"

#include "model/tosite.h"
#include "model/tositevienti.h"
#include "model/tositeviennit.h"
#include "model/tositeliitteet.h"
#include "db/tositetyyppimodel.h"
#include "raportti/raportinkirjoittaja.h"

#include <QSqlQuery>
#include <QDebug>
#include <QMessageBox>
#include <QTemporaryFile>
#include <QPrinter>
#include <QPainter>
#include <QSqlError>

Poistaja::Poistaja(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::Poistaja)
{
    ui->setupUi(this);
}

Poistaja::~Poistaja()
{
    delete ui;
}

bool Poistaja::teepoistot(const Tilikausi &kausi, const QVariantList &poistot)
{
    RaportinKirjoittaja ehdotus = poistoehdotus(kausi, poistot);
    ui->browser->setHtml( ehdotus.html() );
    if( exec() ) {
        // Kirjaillaan poistoja tekemällä uusi tosite ja tallentamalla se

        Tosite *poistotosite = new Tosite(this);
        poistotosite->asetaPvm( kausi.paattyy() );
        poistotosite->asetaTyyppi( TositeTyyppi::POISTOLASKELMA );
        poistotosite->asetaOtsikko( tr("Suunnitelman mukaiset poistot %1").arg(kausi.kausivaliTekstina()));

        poistotosite->liitteet()->lisaa( ehdotus.pdf(), "poistolaskelma.pdf", "poistolaskelma");

        for( auto rivi : poistot) {
            QVariantMap map = rivi.toMap();
            TositeVienti tasevienti;
            TositeVienti tulosvienti;

            Tili* tili = kp()->tilit()->tili(map.value("tili").toInt());
            if( !tili ) {
                QMessageBox::critical(this, tr("Tilikarttavirhe"), tr("Tili %1 puuttuu").arg(map.value("tili").toInt()));
                return false;
            }
            Tili* tulostili = kp()->tilit()->tili( tili->luku("poistotili") );
            if( !tulostili) {
                QMessageBox::critical(this, tr("Tilikarttavirhe"),
                                      tr("Tilille %1 ei ole määritelty kelvollista poistotiliä. Poistojen automaattikirjaamista ei voi tehdä.")
                                      .arg(tili->nimiNumero()));
                return false;
            }

            tasevienti.setPvm( kausi.paattyy() );
            tasevienti.setTyyppi( TositeVienti::POISTO + TositeVienti::VASTAKIRJAUS);
            tasevienti.setTili( tili->numero());

            tulosvienti.setPvm( kausi.paattyy() );
            tulosvienti.setTyyppi( TositeVienti::POISTO);
            tulosvienti.setTili( tulostili->numero());

            tasevienti.setKredit( map.value("poisto").toDouble());
            tulosvienti.setDebet( map.value("poisto").toDouble());

            tasevienti.setKohdennus( map.value("kohdennus").toInt());
            tulosvienti.setKohdennus( map.value("kohdennus").toInt());

            QString selite = map.contains("eraid") ?
                        tr("Tasaeräpoisto %1 ").arg( map.value("nimike").toString())
                      : tr("Menojäännöspoisto %1").arg(tili->nimiNumero());

            tasevienti.setSelite( selite );
            tulosvienti.setSelite( selite );

            poistotosite->viennit()->lisaa(tasevienti);
            poistotosite->viennit()->lisaa(tulosvienti);

        }
        connect( poistotosite, &Tosite::talletettu, this, &Poistaja::poistettu);
        poistotosite->tallenna();

        return true;
    }
    return false;
}



RaportinKirjoittaja Poistaja::poistoehdotus(const Tilikausi &kausi, const QVariantList &poistot)
{
    RaportinKirjoittaja kirjoittaja;

    kirjoittaja.asetaOtsikko("POISTOLASKELMA");
    kirjoittaja.asetaKausiteksti( kausi.paattyy().toString("dd.MM.yyyy"));

    kirjoittaja.lisaaSarake("TUN123456/31.12.9999");
    kirjoittaja.lisaaVenyvaSarake();
    kirjoittaja.lisaaEurosarake();
    kirjoittaja.lisaaSarake("SääntöXX");
    kirjoittaja.lisaaEurosarake();
    kirjoittaja.lisaaEurosarake();

    RaporttiRivi otsikko;
    otsikko.lisaa("Tili/Pvm");
    otsikko.lisaa("Nimike/Kohdennus");
    otsikko.lisaa("Saldo ennen",1, true);
    otsikko.lisaa("Sääntö", 1, true);
    otsikko.lisaa("Poisto",1,true);
    otsikko.lisaa("Saldo jälkeen",1,true);
    kirjoittaja.lisaaOtsake(otsikko);

    int edellinentili = 0;

    for(auto rivi : poistot) {
        QVariantMap map = rivi.toMap();
        Tili* tili = kp()->tilit()->tili( map.value("tili").toInt() );
        if( !tili)
            continue;

        if( tili->numero() != edellinentili) {
            kirjoittaja.lisaaRivi();
            RaporttiRivi tilirivi;
            tilirivi.lisaa( QString::number( tili->numero() ) );
            tilirivi.lisaa( tili->nimi(),5);
            tilirivi.lihavoi();
            kirjoittaja.lisaaRivi(tilirivi);
            edellinentili = tili->numero();
        }


        if( map.contains("eraid")) {
            // Tasaeräpoisto
            RaporttiRivi rr;
            rr.lisaa( map.value("pvm").toDate());
            rr.lisaa( map.value("nimike").toString() );
            rr.lisaa( map.value("ennen").toDouble() );
            rr.lisaa( QString("%1 v").arg(map.value("poistoaika").toInt() / 12));
            rr.lisaa( map.value("poisto").toDouble());
            rr.lisaa( map.value("ennen").toDouble() - map.value("poisto").toDouble());
            kirjoittaja.lisaaRivi(rr);

        } else {
            RaporttiRivi rr;
            rr.lisaa("");
            rr.lisaa( kp()->kohdennukset()->kohdennus( map.value("kohdennus").toInt() ).nimi() );
            rr.lisaa( map.value("ennen").toDouble() );
            rr.lisaa( QString("%1 %").arg(tili->luku("menojaannospoisto")));
            rr.lisaa( map.value("poisto").toDouble());
            rr.lisaa( map.value("ennen").toDouble() - map.value("poisto").toDouble());
            kirjoittaja.lisaaRivi(rr);
        }
    }
    return kirjoittaja;
}

