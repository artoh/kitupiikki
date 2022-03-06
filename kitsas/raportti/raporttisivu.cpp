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

#include <QListWidget>

#include <QDebug>

#include <QHBoxLayout>
#include <QListWidget>
#include <QListWidgetItem>
#include <QStringList>

#include "raporttisivu.h"

#include "pvmraporttiwidget.h"

#include "laskuraportti.h"

#include "db/kirjanpito.h"

#include "tilikarttaraportti.h"
#include "budjettivertailu.h"
#include "tasetulosraportti.h"
#include "alvraporttiwidget.h"

#include "liitepoiminta.h"



RaporttiSivu::RaporttiSivu(QWidget *parent) : KitupiikkiSivu(parent),
    nykyinen(nullptr)
{
    lista = new QListWidget;

    wleiska = new QHBoxLayout;

    QHBoxLayout *paaleiska = new QHBoxLayout;
    paaleiska->addWidget(lista,0);
    paaleiska->addLayout(wleiska, 1);

    setLayout(paaleiska);

    connect( lista, SIGNAL(currentItemChanged(QListWidgetItem*,QListWidgetItem*)), this, SLOT(raporttiValittu(QListWidgetItem*)));

}

void RaporttiSivu::siirrySivulle()
{
    // Laitetaan valinnat listaan
    lista->clear();

    lisaaRaportti(tr("Päiväkirja"),"Päiväkirja",":/pic/Paivakirja64.png");
    lisaaRaportti(tr("Pääkirja"),"Pääkirja",":/pic/Diary64.png");
    lisaaRaportti(tr("Tositeluettelo"),"Tositeluettelo",":/pic/dokumentti.png");
    lisaaRaportti(tr("Tase"), "Tase", ":/pic/tekstisivu.png");
    lisaaRaportti(tr("Tuloslaskelma"), "Tuloslaskelma", ":/pic/tekstisivu.png");

    if( kp()->kohdennukset()->kustannuspaikkoja())
        lisaaRaportti(tr("Kustannuspaikat"), "Kustannuspaikat", ":/pic/kohdennus.png");
    if( kp()->kohdennukset()->projekteja())
        lisaaRaportti(tr("Projektit"), "Projektit", ":/pic/projekti.png");

    // Lisätään muokattavat raportit
    QStringList raporttilista;

    for (QString rnimi : kp()->asetukset()->avaimet("Raportti/") )
    {
        // Kohdennusraportit kuitenkin vain, jos kohdennuksia käytettävissä
        if( kp()->asetukset()->asetus(rnimi).startsWith(":kohdennus") && !kp()->kohdennukset()->kohdennuksia() )
            continue;

        // Raporttilajit: Jos lajillinen raportti (esim. Tase/PMA, tulee listalle kuitenkin vain Tase yhteen kertaan
        if( rnimi.count(QChar('/')) > 1)
            rnimi = rnimi.left( rnimi.lastIndexOf(QChar('/')) );

        if( !raporttilista.contains(rnimi))
            raporttilista.append(rnimi);
    }
    raporttilista.sort(Qt::CaseInsensitive);
    for( const QString& nimi : raporttilista)
    {
        lisaaRaportti( nimi.mid(9), nimi, ":/pic/tekstisivu.png");
    }

    lisaaRaportti(tr("Budjettivertailu"),"Budjettivertailu",":/pic/raha2.png");

    lisaaRaportti(tr("Tase-erittely"),"TaseErittely",":/pic/valilehdet.png");
    lisaaRaportti(tr("Tililuettelo"),"Tilikartta",":/pic/valilehdet.png");

    lisaaRaportti(tr("Laskut"), "Laskut", ":/pic/lasku.png");
    lisaaRaportti(tr("Myynti"),"Myynti",":/pic/suorite.png");

    if( kp()->asetukset()->onko("AlvVelvollinen") )
        lisaaRaportti(tr("Arvonlisäveron erittely"), "AlvErittely", ":/pic/vero.png");

    lisaaRaportti(tr("Tositekooste"), "Tositekooste", ":/pic/arkisto64.png");

    lista->setCurrentItem(lista->item(0));
}

bool RaporttiSivu::poistuSivulta(int /* minne */)
{
    if( nykyinen )
    {
        wleiska->removeWidget( nykyinen );
        delete( nykyinen );
        nykyinen = nullptr;
    }
    return true;

}

void RaporttiSivu::raporttiValittu(QListWidgetItem *item)
{
    if( !item)
        return;

    if( nykyinen )
    {
        wleiska->removeWidget( nykyinen );
        nykyinen->deleteLater();
        nykyinen = nullptr;
    }


    QString raporttinimi = item->data(RAPORTTINIMI).toString();

    if( raporttinimi == "Päiväkirja")
        nykyinen = new PvmRaporttiWidget("paivakirja");
    else if( raporttinimi == "Pääkirja")
        nykyinen = new PvmRaporttiWidget("paakirja");
    else if( raporttinimi == "Tilikartta")
        nykyinen = new TilikarttaRaportti();
    else if( raporttinimi == "Tositeluettelo")
        nykyinen = new PvmRaporttiWidget("tositeluettelo");
    else if( raporttinimi == "TaseErittely")
        nykyinen = new PvmRaporttiWidget("taseerittely");
    else if( raporttinimi == "Laskut")
        nykyinen = new LaskuRaportti();
    else if( raporttinimi == "Myynti")
        nykyinen = new PvmRaporttiWidget("myynti");
    else if( raporttinimi == "Budjettivertailu")
        nykyinen = new Budjettivertailu;
    else if( raporttinimi == "Tase")
        nykyinen = new TaseTulosRaportti("tase");
    else if( raporttinimi == "Tuloslaskelma")
        nykyinen = new TaseTulosRaportti("tulos");
    else if( raporttinimi == "Kustannuspaikat")
        nykyinen = new TaseTulosRaportti("kohdennus");
    else if( raporttinimi == "Projektit")
        nykyinen = new TaseTulosRaportti("projektit");
    else if( raporttinimi == "AlvErittely")
        nykyinen = new AlvRaporttiWidget();
    else if( raporttinimi == "Tositekooste")
        nykyinen = new LiitePoiminta();


    if( nykyinen )
        wleiska->addWidget(nykyinen);

}

void RaporttiSivu::lisaaRaportti(const QString &otsikko, const QString &nimi, const QString &kuvakenimi)
{
    QListWidgetItem *uusi = new QListWidgetItem( QIcon(kuvakenimi), otsikko, lista);
    uusi->setData( RAPORTTINIMI, nimi);
}
