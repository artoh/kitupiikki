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

#include <QScreen>
#include <QGuiApplication>

#include "kitupiikkiikkuna.h"

#include "aloitussivu/aloitussivu.h"
#include "kirjaus/kirjaussivu.h"
#include "maaritys/maarityssivu.h"
#include "selaus/selauswg.h"
#include "raportti/raporttisivu.h"
#include "arkisto/arkistosivu.h"
#include "laskutus/laskusivu.h"
#include "kierto/kiertosivu.h"
#include "alv/alvsivu.h"
#include "kirjaus/tallennettuwidget.h"

#include "db/kirjanpito.h"

#include "onniwidget.h"

#include "lisaikkuna.h"
#include "laskutus/laskudlg/laskudialogitehdas.h"
#include "kirjaus/siirrydlg.h"

#include "tools/inboxlista.h"
#include "tools/devtool.h"

#include "sqlite/sqlitemodel.h"
#include "pilvi/pilvimodel.h"

#include "saldodock/saldodock.h"
#include "laskutus/toimittaja/laskuntoimittaja.h"

KitupiikkiIkkuna::KitupiikkiIkkuna(QWidget *parent) : QMainWindow(parent),
    tallennettuWidget_( new TallennettuWidget(this)),
    aloitussivu( new AloitusSivu(this)),
    laskutussivu( new LaskuSivu(this)),
    selaussivu( new SelausWg(this)),
    kiertosivu( new KiertoSivu(this)),
    raporttisivu( new RaporttiSivu()),
    maarityssivu( new MaaritysSivu()),
    arkistosivu( new ArkistoSivu()),
    alvsivu( new AlvSivu()),
    nykysivu(nullptr),
    onni_(new OnniWidget(this))

{

    connect( Kirjanpito::db(), SIGNAL(tietokantaVaihtui()), this, SLOT(kirjanpitoLadattu()));
    connect(kp(), SIGNAL(perusAsetusMuuttui()), this, SLOT(kirjanpitoLadattu()));

    setWindowIcon(QIcon(":/pic/Possu64.png"));
    setWindowTitle( QString("%1 %2").arg(qApp->applicationName(), qApp->applicationVersion()));

    kirjaussivu =  new KirjausSivu(this, selaussivu);

    pino = new QStackedWidget;
    setCentralWidget(pino);

    lisaaSivut();
    luoHarjoitusDock();
    luoInboxDock();
    addDockWidget(Qt::BottomDockWidgetArea, SaldoDock::dock());

    // Himmennetään ne valinnat, jotka mahdollisia vain kirjanpidon ollessa auki
    for(int i=KIRJAUSSIVU; i<MAARITYSSIVU;i++)
        sivuaktiot[i]->setEnabled(false);

    restoreGeometry( kp()->settings()->value("geometry").toByteArray());
    // Ladataan viimeksi avoinna ollut kirjanpito
    // Vain sqlite
    if( kp()->settings()->contains("Viimeisin") && kp()->settings()->value("Viimeisin").toInt() == 0 )
    {
        QString viimeisin = kp()->settings()->value("Viimeisin").toString();

        // Portable-käsittely
        if( !kp()->portableDir().isEmpty() )
        {
            QDir portableDir( kp()->portableDir());
            viimeisin = QDir::cleanPath( portableDir.absoluteFilePath(viimeisin) );
        }
        // #78 Varmistetaan, että kirjanpito edelleen olemassa (0.7 8.3.2018)
        if( QFile::exists( viimeisin ) )
            kp()->sqlite()->avaaTiedosto(viimeisin, false);
        else
            aloitussivu->kirjanpitoVaihtui();
    }
    else
        aloitussivu->kirjanpitoVaihtui();



    connect( selaussivu, SIGNAL(tositeValittu(int)), this, SLOT(naytaTosite(int)) );
    connect( kiertosivu, &KiertoSivu::tositeValittu, this, &KitupiikkiIkkuna::naytaTosite);
    connect( aloitussivu, SIGNAL(selaus(int,Tilikausi)), this, SLOT(selaaTilia(int,Tilikausi)));
    connect( kirjaussivu, SIGNAL(palaaEdelliselleSivulle()), this, SLOT(palaaSivulta()));

    connect( kp(), &Kirjanpito::onni, this, &KitupiikkiIkkuna::naytaOnni);
    connect( kp(), SIGNAL(naytaTosite(int)), this, SLOT(naytaTosite(int)));
    connect( aloitussivu, SIGNAL(ktpkasky(QString)), this, SLOT(ktpKasky(QString)));

    connect( kp(), &Kirjanpito::perusAsetusMuuttui, this, &KitupiikkiIkkuna::kirjanpitoLadattu);

    // Aktiot kirjaamisella ja selaamisella uudessa ikkunassa

    uusiKirjausAktio = new QAction(QIcon(":/pic/uusitosite.png"), tr("Kirjaa uudessa ikkunassa\tCtrl + W"), this);
    connect( uusiKirjausAktio, SIGNAL(triggered(bool)), this, SLOT(uusiKirjausIkkuna()));
    new QShortcut(QKeySequence(Qt::CTRL + Qt::Key_W), this, SLOT(uusiKirjausIkkuna()), nullptr ,Qt::ApplicationShortcut);

    uusiSelausAktio = new QAction(QIcon(":/pic/Paivakirja64.png"), tr("Selaa uudessa ikkunassa\tShift+F3"), this );
    connect( uusiSelausAktio, SIGNAL(triggered(bool)), this, SLOT(uusiSelausIkkuna()));
    new QShortcut(QKeySequence(Qt::SHIFT + Qt::Key_F3), this, SLOT(uusiSelausIkkuna()), nullptr, Qt::ApplicationShortcut);

    uusiLaskuAktio = new QAction(QIcon(":/pic/lasku.png"), tr("Uusi lasku\tShift+F4"), this);
    connect( uusiLaskuAktio, SIGNAL(triggered(bool)), this, SLOT(uusiLasku()));
    new QShortcut(QKeySequence(Qt::SHIFT + Qt::Key_F4), this, SLOT(uusiLasku()), nullptr, Qt::ApplicationShortcut);

    new QShortcut(QKeySequence("Ctrl+G"), this, SLOT(siirryTositteeseen()), nullptr, Qt::ApplicationShortcut);
    new QShortcut(QKeySequence(Qt::Key_F8), this, SLOT(kirjaaKirjattavienKansiosta()), nullptr, Qt::ApplicationShortcut  );

    connect( new QShortcut(QKeySequence("Ctrl+D"), this), &QShortcut::activated, this, [] () { DevTool *dev = new DevTool(); dev->show(); } );


    toolbar->installEventFilter(this);
    toolbar->setContextMenuPolicy(Qt::PreventContextMenu);

    connect( kp(), &Kirjanpito::tositeTallennettu, tallennettuWidget_, &TallennettuWidget::nayta);
    connect( kp(), &Kirjanpito::piilotaTallennusWidget, tallennettuWidget_, &TallennettuWidget::piiloon);

    LaskunToimittaja::luoInstanssi(this);   // Alustetaan laskujen toimittaja
}

KitupiikkiIkkuna::~KitupiikkiIkkuna()
{
    kp()->settings()->setValue("geometry",saveGeometry());


    QString viimepolku = kp()->kirjanpitoPolku();
    if( viimepolku.isEmpty())
        kp()->settings()->remove("Viimeisin");
    else if( viimepolku.toInt() )
        kp()->settings()->setValue("Viimeisin",viimepolku);
    else if( !kp()->portableDir().isEmpty())
    {
        QDir portableDir( kp()->portableDir());
        kp()->settings()->setValue("Viimeisin", QDir::cleanPath( portableDir.relativeFilePath( kp()->kirjanpitoPolku() ) ));
    }
    else
        kp()->settings()->setValue("Viimeisin", kp()->kirjanpitoPolku() );
}

void KitupiikkiIkkuna::valitseSivu(int mikasivu, bool paluu, bool siirry)
{

    if( nykysivu && !nykysivu->poistuSivulta(mikasivu))
    {
        // Sivulta ei saa poistua!
        // Palautetaan valinta nykyiselle sivulle
        sivuaktiot[ edellisetIndeksit.isEmpty() ? pino->currentIndex() : edellisetIndeksit.top()  ]->setChecked(true);
        return;
    }

    if( !paluu )
        edellisetIndeksit.push( pino->currentIndex() );
    else if( !edellisetIndeksit.isEmpty())
        edellisetIndeksit.pop();

    nykysivu = sivut[mikasivu];
    sivuaktiot[mikasivu]->setChecked(true);

    // Sivu esille
    pino->setCurrentWidget( nykysivu);

    // Laittaa sivun valmiiksi
    if(siirry)
        nykysivu->siirrySivulle();

}


void KitupiikkiIkkuna::kirjanpitoLadattu()
{
    if( kp()->yhteysModel() )
    {
        const QString& nimi = kp()->asetukset()->nimi();

        if( Kirjanpito::db()->onkoHarjoitus())
            setWindowTitle( tr("%1 - Kitsas %2 [Harjoittelu]").arg(nimi, qApp->applicationVersion() ));
        else
            setWindowTitle( tr("%1 - Kitsas %2").arg(nimi, qApp->applicationVersion()));

        harjoitusDock->setVisible( Kirjanpito::db()->onkoHarjoitus());

        for(int i=KIRJAUSSIVU; i<SIVUT_LOPPU;i++)
            sivuaktiot[i]->setEnabled(true);

        sivuaktiot[KIRJAUSSIVU]->setEnabled( kp()->yhteysModel()->onkoOikeutta( YhteysModel::TOSITE_LUONNOS | YhteysModel::TOSITE_MUOKKAUS | YhteysModel::KIERTO_LISAAMINEN ));
        sivuaktiot[SELAUSSIVU]->setEnabled( kp()->yhteysModel()->onkoOikeutta( YhteysModel::TOSITE_SELAUS) );
        sivuaktiot[LASKUTUSSIVU]->setEnabled( kp()->yhteysModel()->onkoOikeutta( YhteysModel::LASKU_SELAUS ));
        sivuaktiot[TULOSTESIVU]->setEnabled( kp()->yhteysModel()->onkoOikeutta( YhteysModel::RAPORTIT ));
        sivuaktiot[ARKISTOSIVU]->setEnabled( kp()->yhteysModel()->onkoOikeutta( YhteysModel::TILINPAATOS | YhteysModel::BUDJETTI ));
        sivuaktiot[ALVSIVU]->setEnabled( kp()->yhteysModel()->onkoOikeutta( YhteysModel::ALV_ILMOITUS ));
        sivuaktiot[KIERTOSIVU]->setVisible(kp()->yhteysModel()->onkoOikeutta(YhteysModel::KIERTO_LISAAMINEN | YhteysModel::KIERTO_SELAAMINEN | YhteysModel::KIERTO_HYVAKSYMINEN | YhteysModel::KIERTO_TARKASTAMINEN) );
    } else {
        for(int i=KIRJAUSSIVU; i < MAARITYSSIVU; i++ )
            sivuaktiot[i]->setEnabled(false);
        setWindowTitle(tr("Kitsas %1").arg(qApp->applicationVersion()));
    }

    edellisetIndeksit.clear();  // Tyhjennetään "selaushistoria"
    sivuaktiot[ALVSIVU]->setVisible( kp()->yhteysModel() && kp()->asetukset()->onko("AlvVelvollinen") );
    valitseSivu(ALOITUSSIVU);
}

void KitupiikkiIkkuna::palaaSivulta()
{
    if( !edellisetIndeksit.isEmpty())
        valitseSivu( edellisetIndeksit.pop(), true );
}

void KitupiikkiIkkuna::selaaTilia(int tilinumero, const Tilikausi& tilikausi)
{
    valitseSivu( SELAUSSIVU, false, false );
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
    LaskuDialogiTehdas::myyntilasku();
}

void KitupiikkiIkkuna::aktivoiSivu(QAction *aktio)
{
    int sivu = aktio->data().toInt();
    valitseSivu(sivu);
}

void KitupiikkiIkkuna::naytaTosite(int tositeid)
{
    // valitseSivu( KIRJAUSSIVU );        
    edellisetIndeksit.push( pino->currentIndex() == KIERTOSIVU ? KIERTOSIVU : SELAUSSIVU );
    pino->setCurrentWidget(kirjaussivu);
    nykysivu = kirjaussivu;
    kirjaussivu->naytaTosite(tositeid);
}

void KitupiikkiIkkuna::ktpKasky(const QString& kasky)
{
    if( kasky.startsWith("maaritys/"))
    {
        valitseSivu( MAARITYSSIVU, true );
        maarityssivu->valitseSivu(kasky.mid(9));
    }
    else if(kasky=="alvilmoitus")
    {
        valitseSivu(ALVSIVU, true);
        alvsivu->ilmoita();
    }
    else if( kasky == "raportit")
        valitseSivu( TULOSTESIVU, true);
    else if( kasky == "kirjaa")
        valitseSivu( KIRJAUSSIVU, true);
    else if( kasky == "uusitilikausi" || kasky=="arkisto" || kasky=="tilinpaatos")
    {
        valitseSivu( ARKISTOSIVU, true);
        if( kasky == "uusitilikausi")
            arkistosivu->uusiTilikausi();
        else if(kasky == "tilinpaatos")
            arkistosivu->tilinpaatosKasky();
            // Varmistaa, että tilinpäätös kohdistuu oikealle tilikaudelle
    } else if( kasky == "inbox") {
        if( qobject_cast<PilviModel*>( kp()->yhteysModel()  ) ) {
            valitseSivu(KIERTOSIVU, true, false);
            kiertosivu->siirrySivulle();
        } else {
            valitseSivu( SELAUSSIVU, true,false);
            selaussivu->naytaSaapuneet();
        }
    } else if( kasky == "outbox") {
        valitseSivu(LASKUTUSSIVU,false,false);
        laskutussivu->outbox();
    } else if( kasky == "huomio") {
        valitseSivu( SELAUSSIVU, false, false);
        selaussivu->naytaHuomioitavat();

    }

}

void KitupiikkiIkkuna::naytaOnni(const QString &teksti,  Kirjanpito::Onni tyyppi)
{    
    onni_->nayta( teksti, tyyppi, (tyyppi == Kirjanpito::Onnistui) ? 5000 : 15000 );

    onni_->move( ( width() - onni_->width()) / 2 ,
                height() - onni_->height());
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

void KitupiikkiIkkuna::kirjaaKirjattavienKansiosta()
{
    valitseSivu(KIRJAUSSIVU, false);
    kirjaussivu->lisaaKirjattavienKansiosta();
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
    uusi->setToolTip( vihje + "\t " + pikanappain );
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
    toolbar = new QToolBar(tr("Valikko"), this);
    toolbar->setToolButtonStyle(Qt::ToolButtonTextUnderIcon);

    // Työkalupalkin namiskat sopeutuvat jonkin verran näytön kokoon, että mahtuvat läppärin näytöllekin
    QScreen *screen = QGuiApplication::primaryScreen();
    if( screen->availableGeometry().height() > 1024)
        toolbar->setIconSize(QSize(64,64));
    else
        toolbar->setIconSize(QSize(32,32));

    toolbar->setStyleSheet("QToolBar {background-color: palette(mid); spacing: 5px; }  QToolBar::separator { border: none; margin-bottom: 16px; }  QToolButton { border: 0px solid lightgray; margin-right: 0px; width: 90%; margin-left: 3px; margin-top: 0px; border-top-left-radius: 6px; border-bottom-left-radius: 6px}  QToolButton:checked {background-color: palette(window); } QToolButton:hover { font-weight: bold; } ");
    toolbar->setMovable(false);

    aktioryhma = new QActionGroup(this);
    lisaaSivu(tr("Aloita"),":/pic/Possu64.png",tr("Erilaisia ohjattuja toimia"),"Home", ALOITUSSIVU, aloitussivu);
    lisaaSivu(tr("Uusi \ntosite"),":/pic/uusitosite.png",tr("Kirjaa uusi tosite"),"Ctrl+N", KIRJAUSSIVU, kirjaussivu);
    lisaaSivu(tr("Selaa"),":/pic/Paivakirja64.png",tr("Selaa kirjauksia aikajärjestyksessä"),"F3", SELAUSSIVU, selaussivu);
    lisaaSivu(tr("Kierto"),":/pic/kierto.svg",tr("Käsittele kierrossa olevia laskuja"),"Ctrl+3", KIERTOSIVU, kiertosivu);
    lisaaSivu(tr("Laskut"),":/pic/lasku.png",tr("Laskuta ja selaa laskuja"),"F4",LASKUTUSSIVU, laskutussivu);
    lisaaSivu(tr("Raportit"),":/pic/print.png",tr("Tulosta erilaisia raportteja"),"Ctrl+5", TULOSTESIVU, raporttisivu);
    lisaaSivu(tr("Tilikaudet"),":/pic/kirja64.png",tr("Tilinpäätös ja arkistot"),"Ctrl+6", ARKISTOSIVU, arkistosivu);
    lisaaSivu(tr("ALV"), ":/pic/vero64.png", tr("Arvonlisäveron ilmoittaminen"), "Ctrl+7",ALVSIVU, alvsivu );
    lisaaSivu(tr("Asetukset"),":/pic/ratas.png",tr("Kirjanpitoon liittyvät määritykset"),"Ctrl+8", MAARITYSSIVU, maarityssivu);

    // Possulla on tonttulakki tuomaanpäivästä loppiaiseen ;)
    if( (QDate::currentDate().month() == 12 && QDate::currentDate().day() >= 21) ||
        (QDate::currentDate().month() == 1 && QDate::currentDate().day() <= 6) )
        sivuaktiot[ALOITUSSIVU]->setIcon(QIcon(":/pic/Joulupossu.png"));


    aktioryhma->actions().first()->setChecked(true);

    connect(aktioryhma, SIGNAL(triggered(QAction*)), this, SLOT(aktivoiSivu(QAction*)));

    QWidget *vali = new QWidget();
    vali->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    toolbar->addWidget(vali);

    QAction *ohjeAktio = new QAction(QIcon(":/pic/ohje.png"),tr("Ohje"), this);
    ohjeAktio->setShortcut( QKeySequence(Qt::Key_F1));
    ohjeAktio->setToolTip("Ohjeet \tF1");
    connect( ohjeAktio, SIGNAL(triggered(bool)), this, SLOT(ohje()));
    toolbar->addAction(ohjeAktio);


    addToolBar(Qt::LeftToolBarArea, toolbar);
}


void KitupiikkiIkkuna::luoHarjoitusDock()
{
    QLabel *teksti = new QLabel("<b>" + tr("Harjoittelutila käytössä") + "</b> " + tr("Voit nopeuttaa ajan kulumista"));
    teksti->setStyleSheet("color: white;");

    QDateEdit *pvmedit = new QDateEdit;
    pvmedit->setDate( QDate::currentDate());
    pvmedit->setStyleSheet("background: palette(window); border-radius: 0px; border: 1px solid black; color: palette(text);");
    pvmedit->setCalendarPopup(true);

    QHBoxLayout *leiska = new QHBoxLayout;
    leiska->addWidget(teksti, 3);
    leiska->addWidget(pvmedit,1, Qt::AlignRight);

    QWidget *wg = new QWidget;
    wg->setLayout(leiska);

    harjoitusDock = new QDockWidget(tr("Harjoittelu"));
    harjoitusDock->setFeatures(QDockWidget::NoDockWidgetFeatures);
    harjoitusDock->setWidget(wg);
    harjoitusDock->setStyleSheet("background: green; border-bottom-left-radius: 10px;");
    harjoitusDock->setTitleBarWidget(new QWidget(this));

    addDockWidget(Qt::TopDockWidgetArea, harjoitusDock);
    connect( pvmedit, SIGNAL(dateChanged(QDate)), Kirjanpito::db(), SLOT(asetaHarjoitteluPvm(QDate)));
    connect( pvmedit, &QDateEdit::dateChanged, Kirjanpito::db(), &Kirjanpito::kirjanpitoaMuokattu );  // Jotta päivittyy ;)
    harjoitusDock->setVisible(false);
}

void KitupiikkiIkkuna::luoInboxDock()
{
    InboxLista *inbox = new InboxLista;

    inboxDock = new QDockWidget(tr("Kirjattavat"));
    inboxDock->setAllowedAreas(Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea );
    inboxDock->setWidget(inbox);

    addDockWidget(Qt::RightDockWidgetArea, inboxDock);
    inboxDock->setVisible( false );
    connect( inbox, &InboxLista::nayta, inboxDock, &QDockWidget::setVisible );

}
