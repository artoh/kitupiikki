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
#include <QVBoxLayout>
#include <QPushButton>

#include <QMessageBox>

#include "maarityssivu.h"

#include "ulkoasumaaritys.h"
#include "perusvalinnat.h"
#include "tilinavaus.h"
#include "tilikarttamuokkaus.h"
#include "kohdennusmuokkaus.h"
#include "raporttimuokkaus.h"
#include "liitetietokaavamuokkaus.h"
#include "emailmaaritys.h"
#include "tuontimaarityswidget.h"
#include "tilikarttaohje.h"
#include "inboxmaaritys.h"
#include "finvoicemaaritys.h"
#include "tilikarttapaivitys.h"

#include "ui_laskumaaritys.h"

#include <QDebug>

MaaritysSivu::MaaritysSivu() :
    KitupiikkiSivu(nullptr), nykyinen(nullptr), nykyItem(nullptr)
{

    lista = new QListWidget;

    lisaaSivu(tr("Näyttöfontti"), ULKOASU, QIcon(":/pic/teksti.png"));
    lisaaSivu(tr("Perusvalinnat"), PERUSVALINNAT, QIcon(":/pic/asetusloota.png"),"perus");
    lisaaSivu(tr("Tililuettelo"), TILIKARTTA, QIcon(":/pic/valilehdet.png"), "tilit");
    lisaaSivu(tr("Kohdennukset"), KOHDENNUS, QIcon(":/pic/kohdennus.png"), "kohdennukset");
    lisaaSivu(tr("Tilinavaus"), TILINAVAUS, QIcon(":/pic/rahaa.png"), "tilinavaus");
    lisaaSivu(tr("Laskutus"), LASKUTUS, QIcon(":/pic/lasku.png"));
//    lisaaSivu("Sähköpostin lähetys", SAHKOPOSTI, QIcon(":/pic/email.png"));
//    lisaaSivu("Verkkolasku", VERKKOLASKU, QIcon(":/pic/verkkolasku.png"));
//    lisaaSivu("Tuonti", TUONTI, QIcon(":/pic/tuotiedosto.png"));
//    lisaaSivu("Kirjattavien kansio", INBOX, QIcon(":/pic/inbox.png"));
//    lisaaSivu("Raportit", RAPORTIT, QIcon(":/pic/print.png"));
//    lisaaSivu("Tilinpäätöksen malli", LIITETIETOKAAVA, QIcon(":/pic/tekstisivu.png"));
//    lisaaSivu("Tilikartan ohje", TILIKARTTAOHJE, QIcon(":/pic/ohje.png"));
    lisaaSivu("Tilikartan päivitys", PAIVITYS, QIcon(":/pic/paivita.png"),"paivita");



    connect( lista, SIGNAL(currentItemChanged(QListWidgetItem*,QListWidgetItem*)), this, SLOT(valitseSivu(QListWidgetItem*)));

    QHBoxLayout *leiska = new QHBoxLayout;
    leiska->addWidget(lista,0);

    sivuleiska = new QVBoxLayout;
    leiska->addLayout(sivuleiska, 1);

    perunappi = new QPushButton(QIcon(":/pic/sulje.png"),tr("Peru"));
    tallennanappi = new QPushButton(QIcon(":/pic/ok.png"),  tr("Tallenna"));
    tallennanappi->setShortcut(QKeySequence(QKeySequence::Save));

    QHBoxLayout *nappiLeiska = new QHBoxLayout;
    nappiLeiska->addStretch();
    nappiLeiska->addWidget(tallennanappi);
    nappiLeiska->addWidget(perunappi);

    sivuleiska->addLayout(nappiLeiska);

    setLayout(leiska);

    connect( perunappi, SIGNAL(clicked(bool)), this, SLOT(peru()));
    connect( tallennanappi, SIGNAL(clicked(bool)), this, SLOT(tallenna()));
    connect( kp(), SIGNAL(tilikausiPaatetty()), this, SLOT(paivitaNakyvat()));

    connect( kp(), &Kirjanpito::perusAsetusMuuttui, this, &MaaritysSivu::paivitaNakyvat);
}

void MaaritysSivu::siirrySivulle()
{
    paivitaNakyvat();   // Piilottaa luettelosta ne valinnat, jotka eivät ole käytössä

    if( lista->currentItem() && !lista->currentItem()->isHidden() )
        valitseSivu( lista->currentItem());
    else
        lista->setCurrentItem( lista->item(0) );
}

bool MaaritysSivu::poistuSivulta(int /* minne */)
{
    if( nykyinen && nykyinen->onkoMuokattu())
    {
        // Nykyistä on muokattu eikä tallennettu
        if( QMessageBox::question(this, tr("Kitupiikki"), tr("Asetuksia on muutettu. Poistutko sivulta tallentamatta tekemiäsi muutoksia?")) != QMessageBox::Yes)
        {
            return false;
        }
    }

    if( nykyinen )
    {
        delete nykyinen;
        nykyinen = nullptr;
    }

    return true;
}

QString MaaritysSivu::ohjeSivunNimi()
{
    if( nykyinen )
        return nykyinen->ohjesivu();
    return QString();
}


void MaaritysSivu::peru()
{
    if( nykyinen )
    {
        if( nykyinen->onkoMuokattu())
        {
            // Jos on muokattu, varmistetaan vielä poistuminen!
            if( QMessageBox::question(this, tr("Kitupiikki"), tr("Asetuksia on muutettu. Poistutko sivulta tallentamatta tekemiäsi muutoksia?")) != QMessageBox::Yes)
            {
                return;
            }
        }

        nykyinen->nollaa();
        tallennanappi->setEnabled( nykyinen->onkoMuokattu());
    }
}

void MaaritysSivu::tallenna()
{
    if( nykyinen )
        nykyinen->tallenna();
    if(nykyinen)
        tallennanappi->setEnabled( nykyinen->onkoMuokattu());
    paivitaNakyvat();
}


void MaaritysSivu::valitseSivu(QListWidgetItem *item)
{

    if( nykyinen)
    {
        if( nykyinen->onkoMuokattu() )
        {
            // Nykyistä on muokattu eikä tallennettu
            if( QMessageBox::question(this, tr("Kitupiikki"), tr("Asetuksia on muutettu. Poistutko sivulta tallentamatta tekemiäsi muutoksia?")) != QMessageBox::Yes)
            {
                nykyItem->setSelected(true);
                return;
            }
        }

        sivuleiska->removeWidget(nykyinen);
        delete nykyinen;
        nykyinen = nullptr;
    }

    int sivu = item->data(Qt::UserRole).toInt();

    if( sivu == ULKOASU)
        nykyinen = new UlkoasuMaaritys;
    else if( sivu == PERUSVALINNAT)
        nykyinen = new Perusvalinnat;
    else if( sivu == TILINAVAUS)
        nykyinen = new Tilinavaus;
    else if( sivu == TILIKARTTA)
        nykyinen = new TilikarttaMuokkaus;
    else if( sivu == KOHDENNUS)
        nykyinen = new KohdennusMuokkaus;
    else if( sivu == RAPORTIT)
        nykyinen = new RaporttiMuokkaus;
    else if( sivu == LIITETIETOKAAVA)
        nykyinen = new LiitetietokaavaMuokkaus;
    else if( sivu == LASKUTUS) {
        nykyinen = new TallentavaMaaritysWidget;
        Ui::LaskuValinnat *ui = new Ui::LaskuValinnat;
        ui->setupUi(nykyinen);
    }
    else if( sivu == TUONTI)
        nykyinen = new TuontiMaaritysWidget;
    else if(sivu == SAHKOPOSTI)
        nykyinen = new EmailMaaritys;
    else if(sivu == TILIKARTTAOHJE)
        nykyinen = new TilikarttaOhje;
    else if(sivu == INBOX)
        nykyinen = new InboxMaaritys;
    else if( sivu == VERKKOLASKU)
        nykyinen = new FinvoiceMaaritys;
    else if( sivu == PAIVITYS)
        nykyinen = new TilikarttaPaivitys;
    else
        nykyinen = new Perusvalinnat;   // Tilipäinen

    sivuleiska->insertWidget(0, nykyinen );
    qApp->processEvents();  // Jotta tulee näkyväksi ja voidaan säätää kokoa
    nykyinen->nollaa();

    item->setSelected(true);
    nykyItem = item;

    perunappi->setVisible( nykyinen->naytetaankoTallennus());
    tallennanappi->setVisible( nykyinen->naytetaankoTallennus());
    tallennanappi->setEnabled( nykyinen->onkoMuokattu() );

    connect( nykyinen, SIGNAL(tallennaKaytossa(bool)), tallennanappi, SLOT(setEnabled(bool)));


}

void MaaritysSivu::valitseSivu(const QString& otsikko)
{
    valitseSivu( item(otsikko) );
}



void MaaritysSivu::paivitaNakyvat()
{
    for(int i=1; i < lista->count(); i++)
        lista->item(i)->setHidden( !kp()->yhteysModel() );

    // Tilinavaus
    // Jos tilit avattavissa eikä avaustilikautta ole vielä päätetty

    item( TILINAVAUS )->setHidden( kp()->asetukset()->luku("Tilinavaus") == 0 || kp()->tilitpaatetty() > kp()->asetukset()->pvm("TilinavausPvm") );
    item( PAIVITYS )->setHidden( !TilikarttaPaivitys::onkoPaivitettavaa() );

}


void MaaritysSivu::lisaaSivu(const QString &otsikko, MaaritysSivu::Sivut sivu, const QIcon &kuvake, const QString& nimi)
{
    QListWidgetItem *item = new QListWidgetItem();
    item->setText( otsikko );
    item->setIcon(kuvake);
    item->setData( Qt::UserRole, QVariant(sivu));
    item->setData( Qt::UserRole + 1, nimi.isEmpty() ? otsikko : nimi);
    lista->addItem( item);
}

QListWidgetItem *MaaritysSivu::item(const QString nimi)
{
    for(int i=0; i < lista->count(); i++)
    {
        QListWidgetItem *item = lista->item(i);
        if( item->data(Qt::UserRole+1).toString() == nimi )
            return item;
    }
    return nullptr;
}

QListWidgetItem *MaaritysSivu::item(int sivu)
{
    for(int i=0; i < lista->count(); i++)
    {
        QListWidgetItem *item = lista->item(i);
        if( item->data(Qt::UserRole).toInt() == sivu )
            return item;
    }
    return nullptr;
}
