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


#include <QSplitter>
#include <QHBoxLayout>

#include "kirjaussivu.h"

#include "kirjauswg.h"
#include "tositewg.h"

KirjausSivu::KirjausSivu() : KitupiikkiSivu()
{
    tositewg = new TositeWg();
    kirjauswg = new KirjausWg(tositewg);

    QSplitter *splitter = new QSplitter(Qt::Vertical);
    splitter->addWidget(tositewg);
    splitter->addWidget(kirjauswg);

    QHBoxLayout *leiska = new QHBoxLayout;
    leiska->addWidget(splitter);

    setLayout(leiska);
}

KirjausSivu::~KirjausSivu()
{
    delete kirjauswg;
}

void KirjausSivu::naytaTosite(int tositeId)
{
    kirjauswg->lataaTosite(tositeId);
}

void KirjausSivu::tyhjenna()
{
    kirjauswg->tyhjenna();
}
