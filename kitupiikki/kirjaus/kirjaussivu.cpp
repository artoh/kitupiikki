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
#include <QMessageBox>
#include <QDebug>

#include "kirjaussivu.h"

#include "kirjauswg.h"
#include "naytaliitewg.h"

#include "db/kirjanpito.h"
#include "db/tositemodel.h"

KirjausSivu::KirjausSivu() : KitupiikkiSivu()
{
    model = kp()->tositemodel();

    liitewg = new NaytaliiteWg();
    kirjauswg = new KirjausWg(model);

    QSplitter *splitter = new QSplitter(Qt::Vertical);
    splitter->addWidget(liitewg);
    splitter->addWidget(kirjauswg);

    QHBoxLayout *leiska = new QHBoxLayout;
    leiska->addWidget(splitter);

    setLayout(leiska);

    connect( liitewg, SIGNAL(lisaaLiite(QString)), kirjauswg, SLOT(lisaaLiite(QString)));
    connect( kirjauswg, SIGNAL(liiteValittu(QByteArray)), liitewg, SLOT(naytaPdf(QByteArray)));
    connect( kirjauswg, SIGNAL(tositeKasitelty()), this, SLOT(tositeKasitelty()));
}

KirjausSivu::~KirjausSivu()
{
    delete kirjauswg;
}

void KirjausSivu::siirrySivulle()
{
    palataanTakaisin_ = false;
    kirjauswg->tyhjenna();
}

bool KirjausSivu::poistuSivulta()
{
    if( model->muokattu())
    {
        if( QMessageBox::question(this, tr("Kitupiikki"), tr("Nykyistä kirjausta on muokattu. Poistutko sivulta tallentamatta tekemiäsi muutoksia?")) != QMessageBox::Yes)
        {
            return false;
        }
    }
    return true;
}

void KirjausSivu::naytaTosite(int tositeId)
{
    palataanTakaisin_ = true;
    kirjauswg->lataaTosite(tositeId);
}

void KirjausSivu::tositeKasitelty()
{
    if( palataanTakaisin_)
        emit palaaEdelliselleSivulle();
}

