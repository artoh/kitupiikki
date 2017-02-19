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
#include <QStackedWidget>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QPushButton>

#include <QMessageBox>

#include "maarityssivu.h"

#include "perusvalinnat.h"
#include "tilinavaus.h"
#include "tositelajit.h"
#include "tilikarttamuokkaus.h"
#include "kohdennusmuokkaus.h"

#include <QDebug>

MaaritysSivu::MaaritysSivu() :
    KitupiikkiSivu(0), nykyinen(0), nykyItem(0)
{

    lista = new QListWidget;

    lisaaSivu("Perusvalinnat", PERUSVALINNAT, QIcon(":/pic/asetusloota.png"));
    lisaaSivu("Tilikartta", TILIKARTTA, QIcon(":/pic/valilehdet.png"));
    lisaaSivu("Tositelajit", TOSITELAJIT, QIcon(":/pic/kansiot.png"));
    lisaaSivu("Kohdennukset", KOHDENNUS, QIcon(":/pic/kohdennus.png"));
    lisaaSivu("Tilinavaus", TILINAVAUS, QIcon(":/pic/rahaa.png"));

    // Nämä vielä paikanpitäjiä...
    lisaaSivu("Tilikaudet", TILIKAUDET, QIcon(":/pic/kirjalaatikko.png"), false );
    lisaaSivu("Arvonlisävero", ALV, QIcon(":/pic/karhu.png"));
    lisaaSivu("Työkalut", TYOKALUT, QIcon(":/pic/vasara.png"), false);

    connect( lista, SIGNAL(itemActivated(QListWidgetItem*)), this, SLOT(valitseSivu(QListWidgetItem*)));

    QHBoxLayout *leiska = new QHBoxLayout;
    leiska->addWidget(lista,0);

    sivuleiska = new QVBoxLayout;
    leiska->addLayout(sivuleiska, 1);

    QHBoxLayout *nappiLeiska = new QHBoxLayout;
    nappiLeiska->addStretch();
    perunappi = new QPushButton(tr("Peru"));
    tallennanappi = new QPushButton( tr("Tallenna"));
    tallennanappi->setShortcut(QKeySequence(QKeySequence::Save));

    nappiLeiska->addWidget(tallennanappi);
    nappiLeiska->addWidget(perunappi);

    sivuleiska->addLayout(nappiLeiska);

    setLayout(leiska);

    connect( perunappi, SIGNAL(clicked(bool)), this, SLOT(peru()));
    connect( tallennanappi, SIGNAL(clicked(bool)), this, SLOT(tallenna()));

}

void MaaritysSivu::siirrySivulle()
{
    paivitaNakyvat();   // Piilottaa luettelosta ne valinnat, jotka eivät ole käytössä
    valitseSivu( lista->item(0) );
}

bool MaaritysSivu::poistuSivulta()
{
    if( nykyinen && nykyinen->onkoMuokattu())
    {
        // Nykyistä on muokattu eikä tallennettu
        if( QMessageBox::question(this, tr("Kitupiikki"), tr("Asetuksia on muutettu. Poistutko sivulta tallentamatta tekemiäsi muutoksia?")) != QMessageBox::Yes)
        {
            return false;
        }
    }

    if( nykyinen )
    {
        delete nykyinen;
        nykyinen = 0;
    }

    return true;
}


void MaaritysSivu::peru()
{
    if( nykyinen )
    {
        nykyinen->nollaa();
        tallennanappi->setEnabled( nykyinen->onkoMuokattu());
    }
}

void MaaritysSivu::tallenna()
{
    if( nykyinen )
    {
        nykyinen->tallenna();
        tallennanappi->setEnabled( nykyinen->onkoMuokattu());
        paivitaNakyvat();
    }

}


void MaaritysSivu::valitseSivu(QListWidgetItem *item)
{
    if( nykyinen)
    {
        if( nykyinen->onkoMuokattu() )
        {
            // Nykyistä on muokattu eikä tallennettu
            if( QMessageBox::question(this, tr("Kitupiikki"), tr("Asetuksia on muutettu. Poistutko sivulta tallentamatta tekemiäsi muutoksia?")) != QMessageBox::Yes)
            {
                lista->setCurrentItem( nykyItem );
                return;
            }
        }

        sivuleiska->removeWidget(nykyinen);
        delete nykyinen;
        nykyinen = 0;
    }

    int sivu = item->data(Qt::UserRole).toInt();


    if( sivu == PERUSVALINNAT)
        nykyinen = new Perusvalinnat;
    else if( sivu == TILINAVAUS)
        nykyinen = new Tilinavaus;
    else if( sivu == TOSITELAJIT)
        nykyinen = new Tositelajit;
    else if( sivu == TILIKARTTA)
        nykyinen = new TilikarttaMuokkaus;
    else if( sivu == KOHDENNUS)
        nykyinen = new KohdennusMuokkaus;
    else
        nykyinen = new Perusvalinnat;   // Tilipäinen

    nykyItem = item;

    if( nykyinen )
    {
        sivuleiska->insertWidget(0, nykyinen );
        qApp->processEvents();  // Jotta tulee näkyväksi ja voidaan säätää kokoa
        nykyinen->nollaa();
    }

    item->setSelected(true);

    perunappi->setVisible( item->data(Qt::UserRole+1).toBool() );
    tallennanappi->setVisible( item->data(Qt::UserRole+1).toBool());

    if( tallennanappi->isVisible())
    {
        tallennanappi->setEnabled( nykyinen->onkoMuokattu() );
        connect( nykyinen, SIGNAL(tallennaKaytossa(bool)), tallennanappi, SLOT(setEnabled(bool)));
    }

}

void MaaritysSivu::paivitaNakyvat()
{
    // ALV
    // Jos merkitty alv-velvolliseksi
    QListWidgetItem *item = lista->item( ALV );
    item->setHidden( !kp()->asetukset()->onko("AlvVelvollinen") );

    // Tilinavaus
    // Jos tilit avattavissa eikä avaustilikautta ole vielä päätetty
    item = lista->item( TILINAVAUS );
    item->setHidden( kp()->asetukset()->luku("Tilinavaus") == 0 || kp()->tilitpaatetty() != kp()->asetukset()->pvm("TilinavausPvm") );

}

void MaaritysSivu::lisaaSivu(const QString &otsikko, MaaritysSivu::Sivut sivu, const QIcon &kuvake, bool tallennaPeruNapit)
{
    QListWidgetItem *item = new QListWidgetItem();
    item->setText( otsikko );
    item->setIcon(kuvake);
    item->setData( Qt::UserRole, QVariant(sivu));
    item->setData( Qt::UserRole+1, QVariant( tallennaPeruNapit ));  // Näytetäänkö alarivin napit
    lista->addItem( item);
}
