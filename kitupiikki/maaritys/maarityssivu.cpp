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

#include "maarityssivu.h"

#include "perusvalinnat.h"
#include "tilinavaus.h"
#include "tositelajit.h"

MaaritysSivu::MaaritysSivu() :
    QWidget(0)
{
    pino = new QStackedWidget;

    pino->addWidget( new Perusvalinnat );
    pino->addWidget( new Tilinavaus );
    pino->addWidget( new Tositelajit);

    lista = new QListWidget;
    lista->addItem("Perusvalinnat");
    lista->addItem("Tilinavaus");
    lista->addItem("Tositelajit");
    lista->addItem("Kustannuspaikat");
    lista->addItem("Projektit");
    lista->addItem("Tilikaudet");
    lista->addItem("Arvonlisävero");
    lista->addItem("Raporttipohjat");

    connect(lista, SIGNAL(currentRowChanged(int)), pino, SLOT(setCurrentIndex(int)));

    QHBoxLayout *leiska = new QHBoxLayout;
    leiska->addWidget(lista,0);
    leiska->addWidget(pino,1);

    setLayout(leiska);

}

void MaaritysSivu::nollaa()
{
    emit nollaaKaikki();
}
