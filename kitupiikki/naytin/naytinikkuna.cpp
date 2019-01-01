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

#include <QAction>
#include <QToolButton>
#include <QToolBar>
#include <QMenu>
#include <QSettings>
#include <QMessageBox>
#include <QSqlQuery>


NaytinIkkuna::NaytinIkkuna(QWidget *parent) : QMainWindow(parent)
{
    setAttribute(Qt::WA_DeleteOnClose);

    resize(800,600);   
    restoreGeometry( kp()->settings()->value("NaytinIkkuna").toByteArray());

    view_ = new NaytinView(this);
    connect( view_, &NaytinView::sisaltoVaihtunut, this, &NaytinIkkuna::sisaltoMuuttui);

    setCentralWidget(view_);

    teeToolbar();

}

NaytinIkkuna::~NaytinIkkuna()
{
    kp()->settings()->setValue("NaytinIkkuna", saveGeometry());
}

void NaytinIkkuna::naytaRaportti(const RaportinKirjoittaja& raportti)
{
    NaytinIkkuna *ikkuna = new NaytinIkkuna;
    ikkuna->show();
    ikkuna->view()->nayta(raportti);
}

void NaytinIkkuna::nayta(const QByteArray& data)
{
    NaytinIkkuna *ikkuna = new NaytinIkkuna;
    ikkuna->show();
    ikkuna->view()->nayta(data);
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

void NaytinIkkuna::naytaLiite(const int tositeId, const int liiteId)
{
    QSqlQuery kysely( QString("SELECT data FROM liite WHERE tosite=%1 AND liiteno=%2")
                      .arg(tositeId).arg(liiteId));
    if( kysely.next() )
    {
        QByteArray data = kysely.value("data").toByteArray();
        nayta(data);
    }
    else
    {
        QMessageBox::critical(nullptr, tr("Virhe liitteen näyttämisessä"),
                              tr("Liitettä %1-%2 ei löydy").arg(tositeId).arg(liiteId));
    }

}

void NaytinIkkuna::sisaltoMuuttui()
{
    setWindowTitle( view()->otsikko() );
    csvAktio_->setVisible( view()->csvKaytossa() );

    raitaAktio_->setVisible( view()->csvKaytossa());
    htmlAktio_->setVisible( view()->htmlKaytossa());

    if( view()->tiedostoPaate() == "jpg"  )
        avaaAktio_->setIcon( QIcon(":/pic/kuva.png"));
    else if( view()->tiedostoPaate() == "pdf")
        avaaAktio_->setIcon( QIcon(":/pic/pdf.png"));
}





void NaytinIkkuna::teeToolbar()
{
    QToolBar *tb = addToolBar(tr("Ikkuna"));
    tb->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);

    tb->addAction(QIcon(":/pic/peru.png"), tr("Sulje"), this, SLOT(close()));
    tb->addSeparator();

    avaaAktio_ = tb->addAction(QIcon(":/pic/pdf.png"), tr("Avaa"));
    connect( avaaAktio_, &QAction::triggered, view(), &NaytinView::avaaOhjelmalla);
    QAction* tallennaAktio = tb->addAction(QIcon(":/pic/tiedostoon.png"), tr("Tallenna"));
    connect(tallennaAktio, &QAction::triggered, view(), &NaytinView::tallenna);

    tb->addSeparator();

    QAction *tulostaAktio = tb->addAction(QIcon(":/pic/tulosta.png"), tr("Tulosta"));
    connect( tulostaAktio, &QAction::triggered, view(), &NaytinView::tulosta);

    tb->addSeparator();

    raitaAktio_ = tb->addAction(QIcon(":/pic/raidoitus.png"), tr("Raidat"));
    connect( raitaAktio_, &QAction::triggered, view_, &NaytinView::raidoita);
    raitaAktio_->setCheckable(true);

    sivunAsetusAktio_ = tb->addAction(QIcon(":/pic/sivunasetukset.png"), tr("Sivun asetukset"));
    connect( sivunAsetusAktio_, &QAction::triggered, view_, &NaytinView::sivunAsetukset );



    tb->addSeparator();

    htmlAktio_ = new QAction(QIcon(":/pic/web.png"), tr("HTML") );
    tb->addAction(htmlAktio_);
    QToolButton *htmlBtn = dynamic_cast<QToolButton*>(tb->widgetForAction(htmlAktio_));
    QMenu *htmlMenu = new QMenu();

    QAction* htmlAvaaAktio = new QAction( QIcon(":/pic/web.png"), tr("Avaa selaimella"));
    connect( htmlAvaaAktio, &QAction::triggered, view(), &NaytinView::avaaHtml );
    htmlMenu->addAction(htmlAvaaAktio);

    QAction *htmlLeikepoytaAktio = new QAction( QIcon(":/pic/edit-paste.png"), tr("Leikepöydälle"));
    connect( htmlLeikepoytaAktio, &QAction::triggered, view(), &NaytinView::htmlLeikepoydalle);
    htmlMenu->addAction(htmlLeikepoytaAktio);

    QAction *htmlTiedostoonAktio = new QAction( QIcon(":/pic/tiedostoon.png"), tr("Tiedostoon"));
    connect( htmlTiedostoonAktio, &QAction::triggered, view(), &NaytinView::tallennaHtml);
    htmlMenu->addAction(htmlTiedostoonAktio);

    htmlBtn->setMenu(htmlMenu);
    htmlBtn->setPopupMode(QToolButton::InstantPopup);

    csvAktio_ = new QAction(QIcon(":/pic/csv.png"), tr("CSV") );
    tb->addAction(csvAktio_);
    QToolButton *csvBtn = dynamic_cast<QToolButton*>( tb->widgetForAction(csvAktio_) );

    QMenu *csvValikko = new QMenu();

    QAction *csvLeikepoydelleAktio = new QAction( QIcon(":/pic/edit-paste.png"), tr("Leikepöydälle") );
    connect( csvLeikepoydelleAktio, &QAction::triggered,  view(), &NaytinView::csvLeikepoydalle );
    csvValikko->addAction(csvLeikepoydelleAktio);
    QAction *csvTallennaAktio = new QAction( QIcon(":/pic/tiedostoon.png"), tr("Tiedostoon"));
    connect( csvTallennaAktio, &QAction::triggered, view(), &NaytinView::tallennaCsv);
    csvValikko->addAction(csvTallennaAktio);
    csvValikko->addSeparator();

    QAction *csvAsetukset = new QAction( QIcon(":/pic/ratas.png"), tr("CSV:n muoto"));
    connect( csvAsetukset, &QAction::triggered, view(), &NaytinView::csvAsetukset);
    csvValikko->addAction(csvAsetukset);

    csvBtn->setMenu(csvValikko);
    csvBtn->setPopupMode(QToolButton::InstantPopup);

}

