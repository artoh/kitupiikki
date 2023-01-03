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
#include "naytinikkuna.h"
#include "naytinview.h"
#include "db/kirjanpito.h"

#include "model/tosite.h"
#include "laskutus/tulostus/laskuntulostaja.h"

#include <QAction>
#include <QToolButton>
#include <QToolBar>
#include <QMenu>
#include <QSettings>
#include <QMessageBox>
#include <QSqlQuery>
#include <QDebug>


NaytinIkkuna::NaytinIkkuna() : QMainWindow(nullptr)
{
    resize(800,600);   
    restoreGeometry( kp()->settings()->value("NaytinIkkuna").toByteArray());

    view_ = new NaytinView(this);
    connect( view_, &NaytinView::sisaltoVaihtunut, this, &NaytinIkkuna::sisaltoMuuttui);

    setCentralWidget(view_);
    teeToolbar();

    setAttribute(Qt::WA_DeleteOnClose);
}

NaytinIkkuna::~NaytinIkkuna()
{
}

void NaytinIkkuna::naytaRaportti(RaportinKirjoittaja raportti)
{
    NaytinIkkuna *ikkuna = new NaytinIkkuna;
    ikkuna->show();
    ikkuna->view()->nayta(raportti);
}

void NaytinIkkuna::naytaRaportti(const RaporttiValinnat &valinnat)
{
    NaytinIkkuna *ikkuna = new NaytinIkkuna;
    ikkuna->show();
    ikkuna->view()->nayta(valinnat);
    connect( ikkuna->view(), &NaytinView::otsikkoVaihtunut, ikkuna, &NaytinIkkuna::setWindowTitle);
    connect( ikkuna->view(), &NaytinView::eiSisaltoa, ikkuna, &NaytinIkkuna::tyhjaRaportti);
}

void NaytinIkkuna::nayta(const QByteArray& data)
{
    NaytinIkkuna *ikkuna = new NaytinIkkuna;
    ikkuna->show();
    ikkuna->view()->nayta(data);
}

void NaytinIkkuna::nayta(const QString &teksti)
{
    NaytinIkkuna *ikkuna = new NaytinIkkuna;
    ikkuna->show();
    ikkuna->view()->nayta(teksti);
}

void NaytinIkkuna::naytaTiedosto(const QString &tiedostonnimi)
{
    QByteArray data;
    QFile tiedosto( tiedostonnimi);
    if( tiedosto.open( QIODevice::ReadOnly) )
    {
        data = tiedosto.readAll();
        tiedosto.close();
        nayta( data );
    }
    else
        QMessageBox::critical(nullptr, tr("Virhe tiedoston näyttämisessä"),
                              tr("Tiedostoa %1 ei voi avata").arg(tiedostonnimi));

}

void NaytinIkkuna::naytaLiite(const int liiteId)
{
    naytaLiite( QString::number(liiteId));
}

void NaytinIkkuna::naytaLiite(const int tositeId, const QString &rooli)
{
    naytaLiite( QString("%1/%2").arg(tositeId).arg(rooli));
}

void NaytinIkkuna::naytaLasku(const int tositeId)
{
    NaytinIkkuna *ikkuna = new NaytinIkkuna;

    KpKysely *kysely = kpk( QString("/liitteet/%1/lasku").arg(tositeId));
    connect( kysely, &KpKysely::vastaus, ikkuna, [ikkuna] (QVariant* data)
        { ikkuna->show(); ikkuna->view()->nayta( data->toByteArray() ); } );
    connect( kysely, &KpKysely::virhe, ikkuna, [ikkuna, tositeId]
        { ikkuna->lataaLaskuTosite(tositeId); });
    kysely->kysy();
}

void NaytinIkkuna::naytaLiite(const QString &hakulauseke)
{
    NaytinIkkuna *ikkuna = new NaytinIkkuna;

    KpKysely *kysely = kpk("/liitteet/" + hakulauseke);
    connect( kysely, &KpKysely::vastaus, ikkuna, [ikkuna] (QVariant* data)
        { ikkuna->show(); ikkuna->view()->nayta( data->toByteArray() ); } );
    kysely->kysy();
}



void NaytinIkkuna::closeEvent(QCloseEvent *event)
{
    kp()->settings()->setValue("NaytinIkkuna", saveGeometry());
    event->accept();
}

void NaytinIkkuna::sisaltoMuuttui()
{
    setWindowTitle( view()->otsikko() );
    csvAktio_->setVisible( view()->csvKaytossa() );

    raitaAktio_->setVisible( view()->csvKaytossa());
    htmlAktio_->setVisible( view()->htmlKaytossa());

    zoomAktio_->setVisible( view()->zoomKaytossa() );
    zoomInAktio_->setVisible( view()->zoomKaytossa() );
    zoomOutAktio_->setVisible( view()->zoomKaytossa() );
    paivitaAktio_->setVisible( view()->paivitaKaytossa());

    if( view()->tiedostoPaate() == "jpg"  )
        avaaAktio_->setIcon( QIcon(":/pic/kuva.png"));
    else if( view()->tiedostoPaate() == "pdf")
        avaaAktio_->setIcon( QIcon(":/pic/pdf.png"));
}

void NaytinIkkuna::lataaLaskuTosite(int tositeId)
{
    Tosite* tosite = new Tosite(this);
    connect( tosite, &Tosite::ladattu, this, &NaytinIkkuna::tositeLadattu);
    tosite->lataa(tositeId);
}

void NaytinIkkuna::tositeLadattu()
{
    Tosite* tosite = qobject_cast<Tosite*>(sender());
    LaskunTulostaja tulostaja(kp(), this);
    show();
    view_->nayta( tulostaja.pdf(*tosite) );
}

void NaytinIkkuna::tyhjaRaportti()
{
    kp()->odotusKursori(false);
    QMessageBox::information(this, tr("Raportti"), tr("Tekemilläsi valinnoilla muodostuu tyhjä raportti"));
    close();
}


void NaytinIkkuna::teeToolbar()
{
    QToolBar *tb = addToolBar(tr("Ikkuna"));
    tb->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);

    QAction *suljeAktio = new QAction(QIcon(":/pic/peru.png"), tr("Sulje"), this);
    connect(suljeAktio, &QAction::triggered, this, &NaytinIkkuna::close);
    suljeAktio->setShortcut(QKeySequence(Qt::Key_Escape));
    tb->addAction(suljeAktio);

    paivitaAktio_ = tb->addAction(QIcon(":/pic/refresh.png"), tr("Päivitä"));
    connect( paivitaAktio_, &QAction::triggered, this, [this] { this->view()->virkista(); });

    tb->addSeparator();


    avaaAktio_ = tb->addAction(QIcon(":/pic/pdf.png"), tr("Avaa"));
    connect( avaaAktio_, &QAction::triggered, this,  [this] {this->view()->avaaOhjelmalla();});

    QAction* tallennaAktio = tb->addAction(QIcon(":/pic/tiedostoon.png"), tr("Tallenna"));
    connect(tallennaAktio, &QAction::triggered, this, [this] { this->view()->tallenna();});

    tb->addSeparator();

    QAction *tulostaAktio = tb->addAction(QIcon(":/pic/tulosta.png"), tr("Tulosta"));
    connect( tulostaAktio, &QAction::triggered, this, [this] {this->view()->tulosta() ;} );

    tb->addSeparator();

    raitaAktio_ = tb->addAction(QIcon(":/pic/raidoitus.png"), tr("Raidat"));
    connect( raitaAktio_, &QAction::triggered, this, [this] (bool raidat) { this->view()->raidoita(raidat); });
    raitaAktio_->setCheckable(true);

    sivunAsetusAktio_ = tb->addAction(QIcon(":/pic/sivunasetukset.png"), tr("Sivun asetukset"));
    connect( sivunAsetusAktio_, &QAction::triggered, this, [this] {this->view()->sivunAsetukset();} );



    tb->addSeparator();

    htmlAktio_ = new QAction(QIcon(":/pic/web.png"), tr("HTML") );
    tb->addAction(htmlAktio_);
    QToolButton *htmlBtn = dynamic_cast<QToolButton*>(tb->widgetForAction(htmlAktio_));
    QMenu *htmlMenu = new QMenu();

    QAction* htmlAvaaAktio = new QAction( QIcon(":/pic/web.png"), tr("Avaa selaimella"));
    connect( htmlAvaaAktio, &QAction::triggered, this,  [this] {this->view()->avaaHtml();} );
    htmlMenu->addAction(htmlAvaaAktio);

    QAction *htmlLeikepoytaAktio = new QAction( QIcon(":/pic/edit-paste.png"), tr("Leikepöydälle"));
    connect( htmlLeikepoytaAktio, &QAction::triggered, this, [this] {this->view()->htmlLeikepoydalle();});
    htmlMenu->addAction(htmlLeikepoytaAktio);

    QAction *htmlTiedostoonAktio = new QAction( QIcon(":/pic/tiedostoon.png"), tr("Tiedostoon"));
    connect( htmlTiedostoonAktio, &QAction::triggered, this, [this] {this->view()->tallennaHtml();});
    htmlMenu->addAction(htmlTiedostoonAktio);

    htmlBtn->setMenu(htmlMenu);
    htmlBtn->setPopupMode(QToolButton::InstantPopup);

    csvAktio_ = new QAction(QIcon(":/pic/csv.png"), tr("CSV") );
    tb->addAction(csvAktio_);
    QToolButton *csvBtn = dynamic_cast<QToolButton*>( tb->widgetForAction(csvAktio_) );

    QMenu *csvValikko = new QMenu();

    QAction *csvLeikepoydelleAktio = new QAction( QIcon(":/pic/edit-paste.png"), tr("Leikepöydälle") );
    connect( csvLeikepoydelleAktio, &QAction::triggered, this, [this] { this->view()->csvLeikepoydalle();});
    csvValikko->addAction(csvLeikepoydelleAktio);
    QAction *csvTallennaAktio = new QAction( QIcon(":/pic/tiedostoon.png"), tr("Tiedostoon"));
    connect( csvTallennaAktio, &QAction::triggered, this, [this] { this->view()->tallennaCsv();});
    csvValikko->addAction(csvTallennaAktio);
    csvValikko->addSeparator();

    QAction *csvAsetukset = new QAction( QIcon(":/pic/ratas.png"), tr("CSV:n muoto"));
    connect( csvAsetukset, &QAction::triggered, this, [this] { this->view()->csvAsetukset();} );
    csvValikko->addAction(csvAsetukset);

    csvBtn->setMenu(csvValikko);
    csvBtn->setPopupMode(QToolButton::InstantPopup);

    tb->addSeparator();

    zoomAktio_ = new QAction( QIcon(":/pic/zoom-fit-width.png"), tr("Sovita"));
    zoomAktio_->setShortcut(QKeySequence(Qt::CTRL + Qt::Key_0));
    connect( zoomAktio_, &QAction::triggered, this, [this] { this->view()->zoomFit();} );

    zoomInAktio_ = new QAction( QIcon(":/pic/zoom-in.png"), tr("Suurenna"), this);
    zoomInAktio_->setShortcut(QKeySequence(Qt::CTRL + Qt::Key_Plus));
    connect( zoomInAktio_, &QAction::triggered, this, [this] { this->view()->zoomIn(); });

    zoomOutAktio_ = new QAction( QIcon(":/pic/zoom-out.png"), tr("Pienennä"), this);
    zoomOutAktio_->setShortcut(QKeySequence(Qt::CTRL + Qt::Key_Minus));
    connect( zoomOutAktio_, &QAction::triggered, this, [this] { this->view()->zoomOut();} );

    tb->addAction(zoomAktio_);
    tb->addAction(zoomInAktio_);
    tb->addAction(zoomOutAktio_);



}

