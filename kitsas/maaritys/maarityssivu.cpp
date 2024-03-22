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

#include "db/yhteysmodel.h"
#include "pilvi/pilvimodel.h"
#include "maarityssivu.h"

#include "ulkoasumaaritys.h"
#include "perusvalinnat.h"
#include "liitemaaritys.h"
#include "tilinavaus/tilinavaus.h"
#include "tilikarttamuokkaus.h"
#include "kohdennusmuokkaus.h"
#include "raportinmuokkaus/raportinmuokkaus.h"
#include "liitetietokaavamuokkaus.h"
#include "emailmaaritys.h"
#include "inboxmaaritys.h"
#include "tilikarttapaivitys.h"
#include "maksutapasivu.h"
#include "tositesarjamaaritys.h"
#include "kayttooikeudet/kayttooikeussivu.h"
#include "verkkolasku/verkkolaskumaaritys.h"
#include "../kierto/kiertomaaritys.h"
#include "palkkatilimaaritys.h"
#include "laskumaaritys.h"
#include "laskutekstit/laskutekstimaaritys.h"
#include "toiminimimaaritys.h"
#include "tilitieto/tilitietomaaritys.h"
#include "bannermaaritys.h"
#include "minamaaritys.h"
#include "veromaaritys.h"
#include "extra/lisapalvelutmaaritys.h"

#include "db/kirjanpito.h"

#include "ui_veromaaritys.h"
#include "ui_oletustilimaaritys.h"

#include <QDebug>
#include <QSizePolicy>

MaaritysSivu::MaaritysSivu() :
    KitupiikkiSivu(nullptr), nykyinen(nullptr), nykyItem(nullptr)
{

    lista = new QListWidget;

    lisaaSivu(tr("Minä"), MINA, "mina", QIcon(":/pic/mies.png"));
    lisaaSivu(tr("Käyttöliittymä"), ULKOASU, "kayttoliittyma", QIcon(":/pic/teksti.png"));
    lisaaSivu(tr("Perusvalinnat"), PERUSVALINNAT, "perusvalinnat", QIcon(":/pic/asetusloota.png"),"perus");
    lisaaSivu(tr("Yhteystiedot ja toiminimet"), YHTEYSTIEDOT, "yhteystiedot", QIcon(":/pic/yhteystiedot.png"),"yhteys");
    lisaaSivu(tr("Liitteiden käsittely"), LIITTEET, "liitteet", QIcon(":/pic/liite.png"),"liitteet");
    lisaaSivu(tr("Käyttöoikeudet"), KAYTTOOIKEUDET, "kayttooikeudet", QIcon(":/pic/asiakkaat.png"),"oikeudet");
    lisaaSivu(tr("Tililuettelo"), TILIKARTTA, "tililuettelo", QIcon(":/pic/valilehdet.png"), "tilit");
    lisaaSivu(tr("Kohdennukset"),KOHDENNUS, "kohdennukset", QIcon(":/pic/kohdennus.png"), "kohdennukset");
    lisaaSivu(tr("Tilinavaus"),TILINAVAUS,  "tilinavaus", QIcon(":/pic/rahaa.png"), "tilinavaus");
    lisaaSivu(tr("Laskutus"), LASKUTUS, "laskutus", QIcon(":/pic/lasku.png"));
    lisaaSivu(tr("Laskujen tekstit"), LASKUTEKSTIT, "laskutekstit", QIcon(":/pic/laskuteksti.png"), "laskutekstit");
    lisaaSivu(tr("Laskujen bannerit"), BANNERIT, "bannerit", QIcon(":/pic/kuva2.png"),"bannerit");
    lisaaSivu(tr("Oletustilit"),OLETUSTILIT,"oletustilit",QIcon(":/pic/uusitosite.png"));
    lisaaSivu(tr("Maksutavat"), MAKSUTAVAT, "maksutavat", QIcon(":/pic/kateinen.png"), "maksutavat");
    lisaaSivu(tr("Tositesarjat"), TOSITESARJAT, "tositesarjat", QIcon(":/pic/arkisto64.png"),"tositesarjat");
    lisaaSivu(tr("Sähköpostin lähetys"), SAHKOPOSTI, "sahkoposti", QIcon(":/pic/email.png"));
    lisaaSivu(tr("Laskujen kierto"), KIERTO, "kierto", QIcon(":/pic/kierto.svg"),"kierto");
    lisaaSivu(tr("Verkkolasku"), VERKKOLASKU,"verkkolaskut",QIcon(":/pic/verkkolasku.png"),"verkkolasku");
    lisaaSivu(tr("Tilitapahtumien haku"), TILITIEDOT, "tilitapahtumat", QIcon(":/pic/verkossa.png"),"tilitapahtumat");
    lisaaSivu(tr("Kirjattavien kansio"), INBOX,"inbox",QIcon(":/pic/inbox.png"));
    lisaaSivu(tr("Verot"), VERO,"veronmaksu", QIcon(":/pic/vero.png"),"vero");
    lisaaSivu(tr("Palkkakirjaustilit"), PALKKAKIRJAUS,"palkkatilit", QIcon(":/pic/yrittaja.png"));
    lisaaSivu(tr("Raportit"), RAPORTIT, "raportit", QIcon(":/pic/print.png"));
    lisaaSivu(tr("Tilinpäätöksen malli"), LIITETIETOKAAVA,"tilinpaatos", QIcon(":/pic/tekstisivu.png"));
    lisaaSivu(tr("Tilikartan päivitys"), PAIVITYS, "paivitys", QIcon(":/pic/paivita.png"),"paivita");
    lisaaSivu(tr("Lisäpalvelut"), LISAPALVELUT, "lisapalvelut", QIcon(":/pic/palat.svg"),"ekstrat");


    // connect( lista, SIGNAL(currentItemChanged(QListWidgetItem*,QListWidgetItem*)), this, SLOT(valitseSivu(QListWidgetItem*)));
    connect(lista, &QListWidget::itemClicked, this, [this] (QListWidgetItem* item) { this->valitseSivu(item); });

    QHBoxLayout *leiska = new QHBoxLayout;
    leiska->addWidget(lista,0);
    lista->setSizePolicy(QSizePolicy(QSizePolicy::Fixed, QSizePolicy::Expanding));

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
        valitseSivu( lista->item(0)->isHidden() ? lista->item(1) : lista->item(0) );
}

bool MaaritysSivu::poistuSivulta(int /* minne */)
{
    if( nykyinen && nykyinen->onkoMuokattu())
    {
        // Nykyistä on muokattu eikä tallennettu
        if( QMessageBox::question(this, tr("Kitsas"), tr("Asetuksia on muutettu. Poistutko sivulta tallentamatta tekemiäsi muutoksia?")) != QMessageBox::Yes)
        {            
            nykyItem->setSelected(true);
            lista->setCurrentItem(nykyItem);
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
    if( nykyItem )
        return nykyItem->data(OHJEOSOITE).toString();
    return QString();
}


void MaaritysSivu::peru()
{
    if( nykyinen )
    {
        if( nykyinen->onkoMuokattu())
        {
            // Jos on muokattu, varmistetaan vielä poistuminen!
            if( QMessageBox::question(this, tr("Kitsas"), tr("Asetuksia on muutettu. Poistutko sivulta tallentamatta tekemiäsi muutoksia?")) != QMessageBox::Yes)
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

    if( !item)
        return;

    if( nykyinen)
    {
        if( nykyinen->onkoMuokattu() )
        {
            // Nykyistä on muokattu eikä tallennettu
            if( QMessageBox::question(this, tr("Kitsas"), tr("Asetuksia on muutettu. Poistutko sivulta tallentamatta tekemiäsi muutoksia?")) != QMessageBox::Yes)
            {
                lista->selectionModel()->select(  lista->model()->index(nykyItem->data(Qt::UserRole).toInt(),0), QItemSelectionModel::SelectCurrent );
                return;
            }
        }

        sivuleiska->removeWidget(nykyinen);
        delete nykyinen;
        nykyinen = nullptr;
    }

    int sivu = item->data(Qt::UserRole).toInt();

    if(sivu == MINA)
        nykyinen = new MinaMaaritys;
    else if( sivu == ULKOASU)
        nykyinen = new UlkoasuMaaritys;
    else if( sivu == PERUSVALINNAT)
        nykyinen = new Perusvalinnat;
    else if( sivu == YHTEYSTIEDOT) {
        nykyinen = new ToiminimiMaaritys;
    }
    else if( sivu == LIITTEET)
        nykyinen = new LiiteMaaritys;
    else if( sivu == TILINAVAUS)
        nykyinen = new Tilinavaus;
    else if( sivu == TILIKARTTA)
        nykyinen = new TilikarttaMuokkaus;
    else if( sivu == KOHDENNUS)
        nykyinen = new KohdennusMuokkaus;
    else if( sivu == RAPORTIT)
        nykyinen = new RaportinMuokkaus;
    else if( sivu == LIITETIETOKAAVA)
        nykyinen = new LiitetietokaavaMuokkaus;
    else if( sivu == LASKUTUS) {
        nykyinen = new LaskuMaaritys;
    } else if( sivu == LASKUTEKSTIT) {
        nykyinen = new LaskuTekstiMaaritys;
    } else if( sivu == BANNERIT) {
        nykyinen = new BannerMaaritys;
    } else if( sivu == OLETUSTILIT) {
        nykyinen = new TallentavaMaaritysWidget;
        Ui::OletusTiliMaaritys *ui = new Ui::OletusTiliMaaritys;
        ui->setupUi(nykyinen);
    } else if( sivu == VERO) {
        nykyinen = new VeroMaaritys;
    }
    else if( sivu == MAKSUTAVAT)
        nykyinen = new MaksutapaSivu;
    else if( sivu == TOSITESARJAT)
        nykyinen = new TositesarjaMaaritys;
    else if(sivu == SAHKOPOSTI)
        nykyinen = new EmailMaaritys;
    else if(sivu == INBOX)
        nykyinen = new InboxMaaritys;
    else if( sivu == VERKKOLASKU)
        nykyinen = new VerkkolaskuMaaritys;
    else if( sivu == PAIVITYS)
        nykyinen = new TilikarttaPaivitys;
    else if( sivu == KAYTTOOIKEUDET)
        nykyinen = new KayttoOikeusSivu;
    else if(sivu == KIERTO)
        nykyinen = new KiertoMaaritys;
    else if(sivu == PALKKAKIRJAUS)
        nykyinen = new PalkkatiliMaaritys;
    else if(sivu == TILITIEDOT)
        nykyinen = new Tilitieto::TilitietoMaaritys();
    else if(sivu == LISAPALVELUT)
        nykyinen = new LisaPalvelutMaaritys;
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
    // Jaoteltu kahteen eri oikeuskategoriaan
    const QList<int> perusvalinnoilla = QList<int>() << PERUSVALINNAT << YHTEYSTIEDOT << LIITTEET << KOHDENNUS
                                                     << LASKUTUS << LASKUTEKSTIT << BANNERIT << SAHKOPOSTI << INBOX << VERKKOLASKU;
    const QList<int> taydetOikeudet = QList<int>() << TILIKARTTA << TILINAVAUS << OLETUSTILIT << KAYTTOOIKEUDET << MAKSUTAVAT << TILITIEDOT
                                                    << TOSITESARJAT << VERO << PALKKAKIRJAUS << RAPORTIT << KIERTO << LISAPALVELUT << VERO << PAIVITYS;
    for(const auto& valinta : perusvalinnoilla)
        item(valinta)->setHidden( !kp()->yhteysModel() || !kp()->yhteysModel()->onkoOikeutta(YhteysModel::PERUSASETUKSET) );
    for(const auto& valinta : taydetOikeudet)
        item(valinta)->setHidden( !kp()->yhteysModel() || !kp()->yhteysModel()->onkoOikeutta(YhteysModel::ASETUKSET) );


    item( MINA )->setHidden( !kp()->pilvi()->kayttaja() );
    // Tilinavaus
    // Jos tilit avattavissa eikä avaustilikautta ole vielä päätetty
    item( TILINAVAUS )->setHidden( kp()->asetukset()->luku("Tilinavaus") == 0 || kp()->tilitpaatetty() > kp()->asetukset()->pvm("TilinavausPvm") ||
                                   !kp()->yhteysModel() || !kp()->yhteysModel()->onkoOikeutta(YhteysModel::ASETUKSET) || !kp()->tilikaudet()->tilikausiPaivalle(kp()->asetukset()->pvm("TilinavausPvm")).alkaa().isValid());
    item( PAIVITYS )->setHidden( !TilikarttaPaivitys::onkoPaivitettavaa() || !kp()->yhteysModel() || !kp()->yhteysModel()->onkoOikeutta(YhteysModel::ASETUKSET));
    item( KAYTTOOIKEUDET)->setHidden( !kp()->yhteysModel() || !kp()->yhteysModel()->onkoOikeutta(YhteysModel::KAYTTOOIKEUDET));
    item( LIITTEET )->setHidden(false); // Liittemääreet aina käytössä
    item( SAHKOPOSTI )->setHidden( kp()->yhteysModel() && !kp()->yhteysModel()->onkoOikeutta(YhteysModel::ASETUKSET) );
        // Jos avoimena olevaan kirjanpitoon ei ole asetusoikeuksia, piilotetaan. Siis silloin kun kirjanpitoa ei ole avoinna, voi muokata konekohtaisia asetuksia.
    item( VERKKOLASKU )->setHidden( !kp()->yhteysModel() ||
           !kp()->yhteysModel()->onkoOikeutta(YhteysModel::LASKU_LAATIMINEN | YhteysModel::LASKU_LAHETTAMINEN | YhteysModel::KIERTO_SELAAMINEN) ||
          (!qobject_cast<PilviModel*>(kp()->yhteysModel()) && !kp()->pilvi()->tilausvoimassa() ) ||
          ( !kp()->yhteysModel()->onkoOikeutta(YhteysModel::ASETUKSET) &&
            !(kp()->asetukset()->luku("FinvoiceKaytossa") > 0 && kp()->yhteysModel()->onkoOikeutta(YhteysModel::LASKU_LAHETTAMINEN))) );

    item( LASKUTUS )->setHidden( !kp()->yhteysModel() || !kp()->yhteysModel()->onkoOikeutta(YhteysModel::LASKU_LAATIMINEN | YhteysModel::LASKU_LAHETTAMINEN) );
    item( LASKUTEKSTIT )->setHidden( !kp()->yhteysModel() || !kp()->yhteysModel()->onkoOikeutta(YhteysModel::LASKU_LAATIMINEN | YhteysModel::LASKU_LAHETTAMINEN) );
    item( BANNERIT )->setHidden( !kp()->yhteysModel() || !kp()->yhteysModel()->onkoOikeutta(YhteysModel::LASKU_LAATIMINEN | YhteysModel::LASKU_LAHETTAMINEN) );
    item( KIERTO )->setHidden( !kp()->yhteysModel() || !kp()->yhteysModel()->onkoOikeutta(YhteysModel::KIERTO_SELAAMINEN | YhteysModel::KIERTO_LISAAMINEN | YhteysModel::KIERTO_TARKASTAMINEN | YhteysModel::KIERTO_HYVAKSYMINEN) );

    // TODO: Disablointi palvelimen mukaan
    item( TILITIEDOT)->setHidden( !kp()->yhteysModel() ||
                                  (!qobject_cast<PilviModel*>(kp()->yhteysModel())) ||
                                   kp()->pilvi()->kbcOsoite().isEmpty() ||
                                   !kp()->yhteysModel()->onkoOikeutta(YhteysModel::ASETUKSET) );
    PilviModel *pilvi = qobject_cast<PilviModel*>(kp()->yhteysModel());
    if( pilvi == nullptr)
        item( KIERTO )->setHidden(true); // Kierto on käytössä vain pilvessä

    item(LISAPALVELUT)->setHidden(!pilvi || !kp()->pilvi()->pilvi().aktiivinen()
                                  || pilvi->onkoOikeutta(YhteysModel::LISAOSA_KAYTTO));
}


void MaaritysSivu::lisaaSivu(const QString &otsikko, MaaritysSivu::Sivut sivu, const QString &ohjesivu, const QIcon &kuvake, const QString& nimi)
{
    QListWidgetItem *item = new QListWidgetItem();
    item->setText( otsikko );
    item->setIcon(kuvake);
    item->setData( SIVUTUNNISTE, QVariant(sivu));
    item->setData( SIVUNNIMI, nimi.isEmpty() ? otsikko : nimi);
    item->setData( OHJEOSOITE, "asetukset/" + ohjesivu );


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
