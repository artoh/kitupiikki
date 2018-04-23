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
#include <QShortcut>

#include <QMenuBar>

#include <QDebug>
#include <QDockWidget>

#include <QDesktopServices>
#include <QUrl>
#include <QFile>

#include "kitupiikkiikkuna.h"

#include "aloitussivu/aloitussivu.h"
#include "kirjaus/kirjaussivu.h"
#include "maaritys/maarityssivu.h"
#include "selaus/selauswg.h"
#include "raportti/raporttisivu.h"
#include "arkisto/arkistosivu.h"
#include "uusikp/uusikirjanpito.h"
#include "laskutus/laskutussivu.h"

#include "db/kirjanpito.h"

#include "onniwidget.h"

#include "lisaikkuna.h"
#include "laskutus/laskudialogi.h"
#include "kirjaus/siirrydlg.h"

KitupiikkiIkkuna::KitupiikkiIkkuna(QWidget *parent) : QMainWindow(parent),
    nykysivu(0)
{

    connect( Kirjanpito::db(), SIGNAL(tietokantaVaihtui()), this, SLOT(kirjanpitoLadattu()));
    connect(kp(), SIGNAL(perusAsetusMuuttui()), this, SLOT(kirjanpitoLadattu()));

    setWindowIcon(QIcon(":/pic/Possu64.png"));
    setWindowTitle( tr("Kitupiikki %1").arg(qApp->applicationVersion()));

    aloitussivu = new AloitusSivu();
    kirjaussivu =  new KirjausSivu(this);
    laskutussivu = new LaskutusSivu();
    selaussivu = new SelausWg();
    maarityssivu = new MaaritysSivu();
    raporttisivu = new RaporttiSivu();
    arkistosivu = new ArkistoSivu();

    pino = new QStackedWidget;
    setCentralWidget(pino);

    lisaaSivut();
    luoHarjoitusDock();


    // Himmennetään ne valinnat, jotka mahdollisia vain kirjanpidon ollessa auki
    for(int i=KIRJAUSSIVU; i<SIVUT_LOPPU;i++)
        sivuaktiot[i]->setEnabled(false);

    QSettings settings;
    restoreGeometry( settings.value("geometry").toByteArray());
    // Ladataan viimeksi avoinna ollut kirjanpito
    if( settings.contains("viimeisin"))
    {
        QString viimeisin = settings.value("viimeisin").toString();
        // #78 Varmistetaan, että kirjanpito edelleen olemassa (0.7 8.3.2018)
        if( QFile::exists( viimeisin ) )
            Kirjanpito::db()->avaaTietokanta(viimeisin);
        else
            aloitussivu->kirjanpitoVaihtui();
    }
    else
        aloitussivu->kirjanpitoVaihtui();



    connect( selaussivu, SIGNAL(tositeValittu(int)), this, SLOT(naytaTosite(int)) );
    connect( aloitussivu, SIGNAL(selaus(int,Tilikausi)), this, SLOT(selaaTilia(int,Tilikausi)));
    connect( kirjaussivu, SIGNAL(palaaEdelliselleSivulle()), this, SLOT(palaaSivulta()));

    connect( kp(), SIGNAL(onni(QString)), this, SLOT(naytaOnni(QString)));
    connect( aloitussivu, SIGNAL(ktpkasky(QString)), this, SLOT(ktpKasky(QString)));

    // Aktiot kirjaamisella ja selaamisella uudessa ikkunassa

    uusiKirjausAktio = new QAction(QIcon(":/pic/uusitosite.png"), tr("Kirjaa uudessa ikkunassa"), this);
    connect( uusiKirjausAktio, SIGNAL(triggered(bool)), this, SLOT(uusiKirjausIkkuna()));
    new QShortcut(QKeySequence(Qt::SHIFT + Qt::Key_F2), this, SLOT(uusiKirjausIkkuna()),0,Qt::ApplicationShortcut);

    uusiSelausAktio = new QAction(QIcon(":/pic/Paivakirja64.png"), tr("Selaa uudessa ikkunassa"), this );
    connect( uusiSelausAktio, SIGNAL(triggered(bool)), this, SLOT(uusiSelausIkkuna()));
    new QShortcut(QKeySequence(Qt::SHIFT + Qt::Key_F3), this, SLOT(uusiSelausIkkuna()), 0, Qt::ApplicationShortcut);

    uusiLaskuAktio = new QAction(QIcon(":/pic/lasku.png"), tr("Uusi lasku"), this);
    connect( uusiLaskuAktio, SIGNAL(triggered(bool)), this, SLOT(uusiLasku()));
    new QShortcut(QKeySequence(Qt::SHIFT + Qt::Key_F4), this, SLOT(uusiLasku()), 0, Qt::ApplicationShortcut);

    new QShortcut(QKeySequence("Ctrl+G"), this, SLOT(siirryTositteeseen()), 0, Qt::ApplicationShortcut);


    toolbar->installEventFilter(this);
    toolbar->setContextMenuPolicy(Qt::PreventContextMenu);



}

KitupiikkiIkkuna::~KitupiikkiIkkuna()
{
    QSettings settings;
    settings.setValue("geometry",saveGeometry());
    settings.setValue("viimeisin", kp()->tiedostopolku() );
}

void KitupiikkiIkkuna::valitseSivu(int mikasivu, bool paluu)
{
    if( !paluu )
        edellisetIndeksit.push( pino->currentIndex() );

    if( nykysivu && !nykysivu->poistuSivulta(mikasivu))
    {
        // Sivulta ei saa poistua!
        // Palautetaan valinta nykyiselle sivulle
        sivuaktiot[ pino->currentIndex() ]->setChecked(true);
        return;
    }

    nykysivu = sivut[mikasivu];
    sivuaktiot[mikasivu]->setChecked(true);

    // Sivu esille
    pino->setCurrentWidget( nykysivu);

    // Laittaa sivun valmiiksi
    nykysivu->siirrySivulle();

}


void KitupiikkiIkkuna::kirjanpitoLadattu()
{
    if( !Kirjanpito::db()->asetus("Nimi").isEmpty())
    {
        if( Kirjanpito::db()->onkoHarjoitus())
            setWindowTitle( tr("%1 - Kitupiikki %2 [Harjoittelu]").arg(Kirjanpito::db()->asetus("Nimi")).arg( qApp->applicationVersion() ));
        else
            setWindowTitle( tr("%1 - Kitupiikki %2").arg(Kirjanpito::db()->asetus("Nimi")).arg(qApp->applicationVersion()));

        harjoitusDock->setVisible( Kirjanpito::db()->onkoHarjoitus());

        for(int i=KIRJAUSSIVU; i<SIVUT_LOPPU;i++)
            sivuaktiot[i]->setEnabled(true);
    }

    valitseSivu(ALOITUSSIVU);
    edellisetIndeksit.clear();  // Tyhjennetään "selaushistoria"
}

void KitupiikkiIkkuna::palaaSivulta()
{
    if( !edellisetIndeksit.isEmpty())
        valitseSivu( edellisetIndeksit.pop(), true );
}

void KitupiikkiIkkuna::selaaTilia(int tilinumero, Tilikausi tilikausi)
{
    valitseSivu( SELAUSSIVU );
    selaussivu->selaa(tilinumero, tilikausi);
}

void KitupiikkiIkkuna::uusiKirjausIkkuna()
{
    LisaIkkuna *ikkuna = new LisaIkkuna;
    ikkuna->kirjaa();
}

void KitupiikkiIkkuna::uusiSelausIkkuna()
{
    LisaIkkuna *ikkuna = new LisaIkkuna;
    ikkuna->selaa();
}

void KitupiikkiIkkuna::uusiLasku()
{
    LaskuDialogi *dlg = new LaskuDialogi(this);
    dlg->show();
}

void KitupiikkiIkkuna::aktivoiSivu(QAction *aktio)
{
    int sivu = aktio->data().toInt();
    if( sivu == KIRJAUSSIVU)
    {
        // Kun kirjaussivu valitaan, tyhjennetään edellisten luettelo jottei
        // tule paluuta kirjauksen jälkeen
        edellisetIndeksit.clear();
        valitseSivu(KIRJAUSSIVU, true);
    }
    else
        valitseSivu(sivu);
}

void KitupiikkiIkkuna::naytaTosite(int tositeid)
{
    valitseSivu( KIRJAUSSIVU );
    kirjaussivu->naytaTosite(tositeid);
}

void KitupiikkiIkkuna::ktpKasky(QString kasky)
{
    if( kasky.startsWith("maaritys/"))
    {
        valitseSivu( MAARITYSSIVU, true );
        maarityssivu->valitseSivu(kasky.mid(9));
    }
    else if( kasky == "raportit")
        valitseSivu( TULOSTESIVU, true);
    else if( kasky == "kirjaa")
        valitseSivu( KIRJAUSSIVU, true);
    else if( kasky == "uusitilikausi" || kasky=="arkisto")
    {
        valitseSivu( ARKISTOSIVU, true);
        if( kasky == "uusitilikausi")
            arkistosivu->uusiTilikausi();
    }
}

void KitupiikkiIkkuna::naytaOnni(const QString &teksti)
{
    OnniWidget *onni = new OnniWidget(this);
    onni->nayta( teksti );
    onni->move( ( width() - onni->width()) / 2 ,
                height() - onni->height());
}

void KitupiikkiIkkuna::ohje()
{
    if( nykysivu )
        kp()->ohje( nykysivu->ohjeSivunNimi() );
    else
        kp()->ohje();
}

void KitupiikkiIkkuna::siirryTositteeseen()
{
    int id = SiirryDlg::tositeId(kp()->paivamaara(), "" );
    if( !id || (nykysivu && !nykysivu->poistuSivulta(KIRJAUSSIVU) ))
    {
        return;
    }
    valitseSivu(KIRJAUSSIVU, false);
    kirjaussivu->naytaTosite( id );

}

void KitupiikkiIkkuna::mousePressEvent(QMouseEvent *event)
{
    // Vähän kokeellista: palataan edelliselle sivulle, jos menty Käy-valinnalla ;)
    if( event->button() == Qt::BackButton )
        palaaSivulta();

    QMainWindow::mousePressEvent(event);
}

bool KitupiikkiIkkuna::eventFilter(QObject *watched, QEvent *event)
{
    // Jos painetaan vasemmalla napilla Kirjausta tai Selausta,
    // tarjotaan mahdollisuus avata toiminto uudessa ikkunassa

    if( watched == toolbar && event->type() == QEvent::MouseButtonPress )
    {
        QMouseEvent* mouse = static_cast<QMouseEvent*>(event);
        if( mouse->button() == Qt::RightButton
            && ( toolbar->actionAt( mouse->pos() ) == sivuaktiot[KIRJAUSSIVU ] || toolbar->actionAt( mouse->pos() ) == sivuaktiot[SELAUSSIVU ] ||
                 toolbar->actionAt( mouse->pos() ) == sivuaktiot[LASKUTUSSIVU ] ))
        {
            QMenu valikko;
            if( toolbar->actionAt( mouse->pos() ) == sivuaktiot[KIRJAUSSIVU ] )
                valikko.addAction( uusiKirjausAktio );
            else if( toolbar->actionAt( mouse->pos() ) == sivuaktiot[SELAUSSIVU ] )
                valikko.addAction( uusiSelausAktio);
            else if( toolbar->actionAt( mouse->pos()) == sivuaktiot[LASKUTUSSIVU] )
                valikko.addAction( uusiLaskuAktio );

            valikko.exec(QCursor::pos() );

            return false;
        }

    }
    return QMainWindow::eventFilter(watched, event);
}

void KitupiikkiIkkuna::closeEvent(QCloseEvent *event)
{
    // Pääikkunan sulkeutuessa sivuikkunatkin suljetaan
    qApp->quit();
    event->accept();
}

QAction *KitupiikkiIkkuna::lisaaSivu(const QString &nimi, const QString &kuva, const QString &vihje, const QString &pikanappain, Sivu sivutunnus,
                                     KitupiikkiSivu *sivu)
{
    QAction *uusi = new QAction( nimi, aktioryhma);
    uusi->setIcon( QIcon(kuva));
    uusi->setStatusTip(vihje);
    uusi->setShortcut(QKeySequence(pikanappain));
    uusi->setCheckable(true);
    uusi->setActionGroup(aktioryhma);
    uusi->setData(sivutunnus);
    toolbar->addAction(uusi);

    sivuaktiot[sivutunnus] = uusi;
    sivut[sivutunnus] = sivu;

    pino->addWidget(sivu);

    return uusi;
}

void KitupiikkiIkkuna::lisaaSivut()
{
    // Luodaan vasemman reunan työkalupalkki
    toolbar = new QToolBar(this);
    toolbar->setToolButtonStyle(Qt::ToolButtonTextUnderIcon);
    toolbar->setIconSize(QSize(64,64));
    toolbar->setStyleSheet("QToolBar {background-color: darkGray; spacing: 5px; }  QToolBar::separator { border: none; margin-bottom: 16px; }  QToolButton { border: 0px solid lightgray; margin-right: 0px; font-size: 8pt; }  QToolButton:checked {background-color: lightGray; } QToolButton:hover { font-size: 9pt; font-weight: bold; } ");
    toolbar->setMovable(false);

    aktioryhma = new QActionGroup(this);
    lisaaSivu("Aloita",":/pic/Possu64.png","Erilaisia ohjattuja toimia","Home", ALOITUSSIVU, aloitussivu);
    lisaaSivu("Uusi\ntosite",":/pic/uusitosite.png","Kirjaa uusi tosite","F2", KIRJAUSSIVU, kirjaussivu);
    lisaaSivu("Selaa",":/pic/Paivakirja64.png","Selaa kirjauksia aikajärjestyksessä","F3", SELAUSSIVU, selaussivu);
    lisaaSivu("Laskut",":/pic/lasku.png","Laskuta ja selaa laskuja","F4",LASKUTUSSIVU, laskutussivu);
    lisaaSivu("Tulosteet",":/pic/print.png","Tulosta erilaisia raportteja","F5", TULOSTESIVU, raporttisivu);
    lisaaSivu("Tilikaudet",":/pic/kirja64.png","Tilinpäätös ja arkistot","F6", ARKISTOSIVU, arkistosivu);
    lisaaSivu("Määritykset",":/pic/ratas.png","Kirjanpitoon liittyvät määritykset","F7", MAARITYSSIVU, maarityssivu);


    aktioryhma->actions().first()->setChecked(true);

    connect(aktioryhma, SIGNAL(triggered(QAction*)), this, SLOT(aktivoiSivu(QAction*)));

    QAction *ohjeAktio = new QAction(QIcon(":/pic/ohje.png"),tr("Käsikirja"), this);
    ohjeAktio->setShortcut( QKeySequence(Qt::Key_F1));
    connect( ohjeAktio, SIGNAL(triggered(bool)), this, SLOT(ohje()));
    toolbar->addAction(ohjeAktio);


    addToolBar(Qt::LeftToolBarArea, toolbar);
}


void KitupiikkiIkkuna::luoHarjoitusDock()
{
    QLabel *teksti = new QLabel("<b>Harjoittelutila käytössä</b> Voit nopeuttaa ajan kulumista");
    teksti->setStyleSheet("color: white;");

    QDateEdit *pvmedit = new QDateEdit;
    pvmedit->setDate( QDate::currentDate());
    pvmedit->setStyleSheet("background: white;");

    QHBoxLayout *leiska = new QHBoxLayout;
    leiska->addWidget(teksti, 3);
    leiska->addWidget(pvmedit,1, Qt::AlignRight);

    QWidget *wg = new QWidget;
    wg->setLayout(leiska);

    harjoitusDock = new QDockWidget;
    harjoitusDock->setFeatures(QDockWidget::NoDockWidgetFeatures);
    harjoitusDock->setWidget(wg);
    harjoitusDock->setStyleSheet("background: green");
    harjoitusDock->setTitleBarWidget(new QWidget(this));

    addDockWidget(Qt::TopDockWidgetArea, harjoitusDock);
    connect( pvmedit, SIGNAL(dateChanged(QDate)), Kirjanpito::db(), SLOT(asetaHarjoitteluPvm(QDate)));
    connect( pvmedit, SIGNAL(dateChanged(QDate)), aloitussivu, SLOT(siirrySivulle()));  // Jotta päivittyy ;)
    harjoitusDock->setVisible(false);
}
