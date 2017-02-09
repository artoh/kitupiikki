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

#include <QDebug>

MaaritysSivu::MaaritysSivu() :
    KitupiikkiSivu(0), nykyinen(0), nykyItem(0)
{

    lista = new QListWidget;

    lisaaSivu("Perusvalinnat", PERUSVALINNAT);
    lisaaSivu("Tilinavaus", TILINAVAUS);
    lisaaSivu("Tositelajit", TOSITELAJIT);

    connect( lista, SIGNAL(itemActivated(QListWidgetItem*)), this, SLOT(valitseSivu(QListWidgetItem*)));

    QHBoxLayout *leiska = new QHBoxLayout;
    leiska->addWidget(lista,0);

    sivuleiska = new QVBoxLayout;
    leiska->addLayout(sivuleiska, 1);

    QHBoxLayout *nappiLeiska = new QHBoxLayout;
    nappiLeiska->addStretch();
    QPushButton *perunappi = new QPushButton(tr("Peru"));
    QPushButton *tallennanappi = new QPushButton( tr("Tallenna"));
    nappiLeiska->addWidget(perunappi);
    nappiLeiska->addWidget(tallennanappi);

    sivuleiska->addLayout(nappiLeiska);

    setLayout(leiska);

    connect( perunappi, SIGNAL(clicked(bool)), this, SLOT(peru()));
    connect( tallennanappi, SIGNAL(clicked(bool)), this, SLOT(tallenna()));



}

void MaaritysSivu::nollaa()
{
    emit nollaaKaikki();
}

void MaaritysSivu::peru()
{
    if( nykyinen )
        nykyinen->nollaa();
}

void MaaritysSivu::tallenna()
{
    if( nykyinen )
        nykyinen->tallenna();
}

void MaaritysSivu::aktivoiSivu(int sivu)
{

    if( nykyinen)
    {
        if( nykyinen->onkoMuokattu() )
        {
            if( QMessageBox::question(this, tr("Kitupiikki"), tr("Asetuksia on muutettu. Poistutko sivulta tallentamatta tekemiäsi muutoksia?")) != QMessageBox::Yes)
                return;
        }

        sivuleiska->removeWidget(nykyinen);
        delete nykyinen;
        nykyinen = 0;
    }

    if( sivu == 0)
        nykyinen = new Perusvalinnat;
    else if( sivu == 1)
        nykyinen = new Tilinavaus;
    else if( sivu == 2)
        nykyinen = new Tositelajit;

    if( nykyinen )
    {
        sivuleiska->insertWidget(0, nykyinen );
        nykyinen->nollaa();
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

    nykyItem = item;

    if( nykyinen )
    {
        sivuleiska->insertWidget(0, nykyinen );
        nykyinen->nollaa();
    }

    item->setSelected(true);
}

void MaaritysSivu::lisaaSivu(const QString &otsikko, MaaritysSivu::Sivut sivu, const QIcon &kuvake)
{
    QListWidgetItem *item = new QListWidgetItem();
    item->setText( otsikko );
    item->setIcon(kuvake);
    item->setData( Qt::UserRole, QVariant(sivu));
    lista->addItem( item);
}
