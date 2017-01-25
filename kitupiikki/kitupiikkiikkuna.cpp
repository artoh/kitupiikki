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

#include <QStackedWidget>
#include <QToolBar>
#include <QSettings>
#include <QStatusBar>
#include <QFileDialog>
#include <QDateEdit>
#include <QMouseEvent>

#include <QMenuBar>

#include <QDebug>

#include "kitupiikkiikkuna.h"

#include "aloitussivu/aloitussivu.h"
#include "kirjaus/kirjaussivu.h"
#include "maaritys/maarityssivu.h"
#include "selaus/selauswg.h"
#include "uusikp/uusikirjanpito.h"

#include "db/kirjanpito.h"

KitupiikkiIkkuna::KitupiikkiIkkuna(QWidget *parent) : QMainWindow(parent)
{

    connect( Kirjanpito::db(), SIGNAL(tietokantaVaihtui()), this, SLOT(kirjanpitoLadattu()));

    setWindowIcon(QIcon(":/pic/Possu64.png"));
    luoPalkkiJaSivuAktiot();
    luoStatusBar();

    // Nämä valikot ovat tässä lähinnä käyttöliittymätestin kannalta ;)
    menuBar()->addMenu("Kirjanpito");
    menuBar()->addMenu("Tosite");
    menuBar()->addMenu("Raportit");

    aloitussivu = new AloitusSivu();
    aloitussivu->lataaAloitussivu();
    connect( aloitussivu, SIGNAL(toiminto(QString)), this, SLOT(toiminto(QString)));

    kirjaussivu = new KirjausSivu();
    selaussivu = new SelausWg();
    maarityssivu = new MaaritysSivu();


    pino = new QStackedWidget;
    pino->addWidget( aloitussivu);
    pino->addWidget( kirjaussivu );
    pino->addWidget( selaussivu );
    pino->addWidget( maarityssivu);
    setCentralWidget(pino);

    statusBar()->showMessage("Tervetuloa",1000);

    // Himmennetään ne valinnat, jotka mahdollisia vain kirjanpidon ollessa auki
    for(int i=KIRJAUSSIVU; i<OHJESIVU;i++)
        sivuaktiot[i]->setEnabled(false);

    QSettings settings;
    restoreGeometry( settings.value("geometry").toByteArray());
    // Ladataan viimeksi avoinna ollut kirjanpito
    if( settings.contains("viimeisin"))
        Kirjanpito::db()->avaaTietokanta(settings.value("viimeisin").toString());

    connect( selaussivu, SIGNAL(tositeValittu(int)), this, SLOT(naytaTosite(int)) );


}

KitupiikkiIkkuna::~KitupiikkiIkkuna()
{
    QSettings settings;
    settings.setValue("geometry",saveGeometry());
    settings.setValue("viimeisin", Kirjanpito::db()->hakemisto().absoluteFilePath("kitupiikki.sqlite"));
}

void KitupiikkiIkkuna::valitseSivu(int mikasivu)
{
    if( mikasivu == ALOITUSSIVU)
    {
        aloitussivu->lataaAloitussivu();
        pino->setCurrentWidget( aloitussivu );
    }
    else if( mikasivu == OHJESIVU)
    {
        aloitussivu->lataaOhje();
        pino->setCurrentWidget( aloitussivu);
    }
    else if( mikasivu == KIRJAUSSIVU)
    {
        kirjaussivu->tyhjenna();
        pino->setCurrentWidget( kirjaussivu);
    }
    else if( mikasivu == MAARITYSSIVU)
    {
        maarityssivu->nollaa();
        pino->setCurrentWidget( maarityssivu);
    }
    else if( mikasivu == PAIVAKIRJASIVU )
    {
        pino->setCurrentWidget( selaussivu);
    }

}

void KitupiikkiIkkuna::toiminto(const QString &toiminto)
{
    if( toiminto == "uusi")
    {
        QString uusitiedosto = UusiKirjanpito::aloitaUusiKirjanpito();
        if( !uusitiedosto.isEmpty())
            Kirjanpito::db()->avaaTietokanta(uusitiedosto + "/kitupiikki.sqlite");
    }
    else if( toiminto == "avaa")
    {
        QString polku = QFileDialog::getOpenFileName(this, "Avaa kirjanpito",
                                                     QDir::homePath(),"Kirjanpito (kitupiikki.sqlite)");
        if( !polku.isEmpty())
            Kirjanpito::db()->avaaTietokanta(polku);
    }
    else if( toiminto.startsWith("/"))
    {
        // Avataan yksi viimeisimmistä tietokannoista
        Kirjanpito::db()->avaaTietokanta(toiminto);
    }
}

void KitupiikkiIkkuna::kirjanpitoLadattu()
{
    if( !Kirjanpito::db()->asetus("nimi").isEmpty())
    {
        if( Kirjanpito::db()->onkoHarjoitus())
            setWindowTitle( tr("%1 - Kitupiikki [Harjoittelu]").arg(Kirjanpito::db()->asetus("nimi")));
        else
            setWindowTitle( tr("%1 - Kitupiikki").arg(Kirjanpito::db()->asetus("nimi")));

        harjoituspvmEdit->setVisible( Kirjanpito::db()->onkoHarjoitus());

        for(int i=KIRJAUSSIVU; i<OHJESIVU;i++)
            sivuaktiot[i]->setEnabled(true);

        selaussivu->alusta();
    }

    valitseSivu(ALOITUSSIVU);
}

void KitupiikkiIkkuna::aktivoiSivu(QAction *aktio)
{
    int sivu = aktio->data().toInt();
    qDebug() << "Sivulle " << sivu;
    valitseSivu(sivu);
}

void KitupiikkiIkkuna::naytaTosite(int tositeid)
{
    qDebug() << "Nayta " << tositeid;
    kirjaussivu->naytaTosite(tositeid);
    pino->setCurrentWidget( kirjaussivu );
}

void KitupiikkiIkkuna::mousePressEvent(QMouseEvent *event)
{
    // Vähän kokeellista: laitetaan back-napilla selaukseen
    if( event->button() == Qt::BackButton )
        pino->setCurrentWidget( selaussivu );
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
    luosivuAktio("Selaa",":/pic/Paivakirja64.png","Selaa kirjauksia aikajärjestyksessä","F3", PAIVAKIRJASIVU);
 //   luosivuAktio("Pääkirja",":/pic/Diary64.png","Selaa kirjauksia tileittäin","F4", PAAKIRJASIVU);
    luosivuAktio("Tulosteet",":/pic/print.png","Tulosta erilaisia raportteja","F5", TULOSTESIVU);
    luosivuAktio("Määritykset",":/pic/ratas.png","Kirjanpitoon liittyvät määritykset","F6", MAARITYSSIVU);
    luosivuAktio("Ohje",":/pic/ohje.png","Kitupiikin ohjeet","F1", OHJESIVU);
    aktioryhma->actions().first()->setChecked(true);

    connect(aktioryhma, SIGNAL(triggered(QAction*)), this, SLOT(aktivoiSivu(QAction*)));

    addToolBar(Qt::LeftToolBarArea, toolbar);
}

void KitupiikkiIkkuna::luoStatusBar()
{
    harjoituspvmEdit = new QDateEdit();
    harjoituspvmEdit->setDate(QDate::currentDate());

    statusBar()->addPermanentWidget(harjoituspvmEdit);
    connect( harjoituspvmEdit, SIGNAL(dateChanged(QDate)), Kirjanpito::db(), SLOT(asetaHarjoitteluPvm(QDate)));
    // Päivän vaihtamisen pitäisi myös päivittää näytettävä aloitussivu
    harjoituspvmEdit->setVisible(false);
}
