/*
   Copyright (C) 2018 Arto Hyvättinen

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

#include "lisaikkuna.h"

#include "kirjaus/kirjaussivu.h"
#include "selaus/selauswg.h"

#include "db/kirjanpito.h"

#include <QShortcut>
#include <QSettings>

LisaIkkuna::LisaIkkuna(QWidget *parent) : QMainWindow(parent)
{
    setAttribute(Qt::WA_DeleteOnClose);
    connect( kp(), SIGNAL(tietokantaVaihtui()), this, SLOT(close()));

    restoreGeometry(  kp()->settings()->value("LisaIkkuna").toByteArray() );

    new QShortcut( QKeySequence(Qt::Key_F1), this, SLOT(ohje()));
}

LisaIkkuna::~LisaIkkuna()
{
    kp()->settings()->setValue("LisaIkkuna", saveGeometry());
}

KirjausSivu* LisaIkkuna::kirjaa(int tositeId, int tyyppi)
{
    KirjausSivu *sivu = new KirjausSivu(nullptr, nullptr);
    setCentralWidget( sivu );
    show();
    sivu->siirrySivulle();
    sivu->naytaTosite(tositeId, tyyppi);


    connect( sivu, SIGNAL(palaaEdelliselleSivulle()), this, SLOT(close()));


    ohjesivu = sivu->ohjeSivunNimi();

    setWindowTitle(tr("%1 - Kirjaus").arg(kp()->asetukset()->nimi()));

    return sivu;
}

void LisaIkkuna::selaa()
{
    SelausWg *sivu = new SelausWg();
    sivu->alusta();
    setCentralWidget(sivu);
    sivu->siirrySivulle();
    show();

    ohjesivu = sivu->ohjeSivunNimi();
    connect( sivu, SIGNAL(tositeValittu(int)), this, SLOT(naytaTosite(int)));
    setWindowTitle(tr("%1 - Selaus").arg(kp()->asetukset()->nimi()));
    new QShortcut( QKeySequence(Qt::Key_Escape), this, SLOT(close()));
}

void LisaIkkuna::naytaTosite(int tositeId)
{
    LisaIkkuna *uusi = new LisaIkkuna;
    uusi->kirjaa(tositeId);
}

void LisaIkkuna::ohje()
{
    kp()->ohje( ohjesivu );
}
