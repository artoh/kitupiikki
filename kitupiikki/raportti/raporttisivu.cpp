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

#include "paivakirjaraportti.h"
#include "paakirjaraportti.h"
#include "tositeluetteloraportti.h"
#include "taseerittely.h"
#include "laskuraportti.h"

#include "db/kirjanpito.h"

#include "muokattavaraportti.h"
#include "tilikarttaraportti.h"

RaporttiSivu::RaporttiSivu(QWidget *parent) : KitupiikkiSivu(parent),
    nykyinen(0)
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

    lisaaRaportti("Päiväkirja","Päiväkirja",":/pic/Paivakirja64.png");
    lisaaRaportti("Pääkirja","Pääkirja",":/pic/Diary64.png");
    lisaaRaportti("Tositeluettelo","Tositeluettelo",":/pic/dokumentti.png");

    // Lisätään muokattavat raportit
    QStringList raporttilista;

    foreach (QString rnimi, kp()->asetukset()->avaimet("Raportti/") )
    {
        raporttilista << rnimi;
    }
    raporttilista.sort(Qt::CaseInsensitive);
    for( QString nimi : raporttilista)
    {
        lisaaRaportti( nimi.mid(9), nimi, ":/pic/tekstisivu.png");
    }


    lisaaRaportti("Tase-erittely","TaseErittely",":/pic/valilehdet.png");
    lisaaRaportti("Tililuettelo","Tilikartta",":/pic/valilehdet.png");
    lisaaRaportti("Laskut", "Laskut", ":/pic/lasku.png");
    lista->setCurrentItem(lista->item(0));
}

bool RaporttiSivu::poistuSivulta(int /* minne */)
{
    if( nykyinen )
    {
        wleiska->removeWidget( nykyinen );
        delete( nykyinen );
        nykyinen = 0;
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
        nykyinen = 0;
    }


    QString raporttinimi = item->data(RAPORTTINIMI).toString();

    if( raporttinimi == "Päiväkirja")
        nykyinen = new PaivakirjaRaportti();
    else if( raporttinimi == "Pääkirja")
        nykyinen = new PaakirjaRaportti();
    else if( raporttinimi.startsWith("Raportti/"))
        nykyinen = new MuokattavaRaportti( raporttinimi.mid(9));
    else if( raporttinimi == "Tilikartta")
        nykyinen = new TilikarttaRaportti();
    else if( raporttinimi == "Tositeluettelo")
        nykyinen = new TositeluetteloRaportti();
    else if( raporttinimi == "TaseErittely")
        nykyinen = new TaseErittely();
    else if( raporttinimi == "Laskut")
        nykyinen = new LaskuRaportti();


    if( nykyinen )
        wleiska->addWidget(nykyinen);

}

void RaporttiSivu::lisaaRaportti(const QString &otsikko, const QString &nimi, const QString &kuvakenimi)
{
    QListWidgetItem *uusi = new QListWidgetItem( QIcon(kuvakenimi), otsikko, lista);
    uusi->setData( RAPORTTINIMI, nimi);
}
