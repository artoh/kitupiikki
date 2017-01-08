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

#include <QAction>
#include <QActionGroup>

#include <QToolBar>
#include <QSettings>
#include <QStatusBar>

#include <QDebug>

#include "kitupiikkiikkuna.h"

#include "aloitussivu/aloitussivu.h"

#include "uusikp/uusikirjanpito.h"

KitupiikkiIkkuna::KitupiikkiIkkuna(QWidget *parent) : QMainWindow(parent)
{
    setWindowIcon(QIcon(":/pic/Possu64.png"));
    luoPalkkiJaSivuAktiot();

    aloitussivu = new AloitusSivu();
    connect( aloitussivu, SIGNAL(toiminto(QString)), this, SLOT(toiminto(QString)));

    QSettings settings;
    restoreGeometry( settings.value("geometry").toByteArray());

    valitseSivu(ALOITUSSIVU);

    statusBar()->showMessage("Tervetuloa",1000);

    for(int i=KIRJAUSSIVU; i<OHJESIVU;i++)
        sivuaktiot[i]->setEnabled(false);

}

KitupiikkiIkkuna::~KitupiikkiIkkuna()
{
    QSettings settings;
    settings.setValue("geometry",saveGeometry());
}

void KitupiikkiIkkuna::valitseSivu(int mikasivu)
{
    setCentralWidget( aloitussivu );
    if( mikasivu == ALOITUSSIVU)
        aloitussivu->lataaAloitussivu();
    else if( mikasivu == OHJESIVU)
        aloitussivu->lataaOhje();
}

void KitupiikkiIkkuna::toiminto(const QString &toiminto)
{
    if( toiminto == "uusi")
    {
        QString uusitiedosto = UusiKirjanpito::aloitaUusiKirjanpito();
    }
}

void KitupiikkiIkkuna::aktivoiSivu(QAction *aktio)
{
    int sivu = aktio->data().toInt();
    qDebug() << "Sivulle " << sivu;
    valitseSivu(sivu);
}

QAction *KitupiikkiIkkuna::luosivuAktio(const QString &nimi, const QString &kuva, const QString &vihje, const QString &pikanappain, Sivu sivu)
{
    QAction *uusi = new QAction( nimi, aktioryhma);
    uusi->setIcon( QIcon(kuva));
    uusi->setStatusTip(vihje);
    uusi->setShortcut(QKeySequence(pikanappain));
    uusi->setCheckable(true);
    uusi->setActionGroup(aktioryhma);
    uusi->setData(sivu);
    toolbar->addAction(uusi);
    sivuaktiot[sivu] = uusi;

    return uusi;
}

void KitupiikkiIkkuna::luoPalkkiJaSivuAktiot()
{
    // Luodaan vasemman reunan työkalupalkki
    toolbar = new QToolBar(this);
    toolbar->setToolButtonStyle(Qt::ToolButtonTextUnderIcon);
    toolbar->setIconSize(QSize(64,64));
    toolbar->setStyleSheet("QToolBar {background-color: darkGray; spacing: 5px; }  QToolBar::separator { border: none; margin-bottom: 16px; }  QToolButton { border: 0px solid lightgray; margin-right: 0px; font-size: 8pt; }  QToolButton:checked {background-color: lightGray; } QToolButton:hover { font-size: 9pt; font-weight: bold; } ");
    toolbar->setMovable(false);

    aktioryhma = new QActionGroup(this);
    luosivuAktio("Aloita",":/pic/Possu64.png","Erilaisia ohjattuja toimia","Home", ALOITUSSIVU);
    luosivuAktio("Uusi\ntosite",":/pic/uusitosite.png","Kirjaa uusi tosite","Ctrl+N", KIRJAUSSIVU);
    luosivuAktio("Päiväkirja",":/pic/Paivakirja64.png","Selaa kirjauksia aikajärjestyksessä","F3", PAIVAKIRJASIVU);
    luosivuAktio("Pääkirja",":/pic/Diary64.png","Selaa kirjauksia tileittäin","F4", PAAKIRJASIVU);
    luosivuAktio("Tulosteet",":/pic/print.png","Tulosta erilaisia raportteja","F5", TULOSTESIVU);
    luosivuAktio("Määritykset",":/pic/ratas.png","Kirjanpitoon liittyvät määritykset","F6", MAARITYSSIVU);
    luosivuAktio("Ohje",":/pic/ohje.png","Kitupiikin ohjeet","F1", OHJESIVU);
    aktioryhma->actions().first()->setChecked(true);

    connect(aktioryhma, SIGNAL(triggered(QAction*)), this, SLOT(aktivoiSivu(QAction*)));

    addToolBar(Qt::LeftToolBarArea, toolbar);
}
