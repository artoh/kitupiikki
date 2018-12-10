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

#include "laskunmaksudialogi.h"
#include "ui_laskunmaksudialogi.h"
#include "db/jsonkentta.h"
#include "db/kirjanpito.h"
#include "kirjaus/ehdotusmodel.h"
#include "naytin/naytinikkuna.h"
#include "lisaikkuna.h"
#include "kirjaus/kirjaussivu.h"

#include <QSqlQuery>
#include <QMessageBox>
#include <QDesktopServices>
#include <QUrl>

LaskunMaksuDialogi::LaskunMaksuDialogi(KirjausWg *kirjauswg) :
    QDialog(kirjauswg),
    kirjaaja(kirjauswg),
    ui(new Ui::LaskunMaksuDialogi)
{
    ui->setupUi(this);

    ui->tabBar->addTab(tr("Myyntilaskut"));
    ui->tabBar->addTab(tr("Ostolaskut"));

    laskut = new LaskutModel(this);
    ostolaskut = new OstolaskutModel(this);

    laskut->lataaAvoimet();
    ostolaskut->lataaAvoimet();

    proxy = new QSortFilterProxyModel(this);
    proxy->setSourceModel( laskut );
    proxy->setDynamicSortFilter(true);
    proxy->setSortRole(Qt::EditRole);

    ui->laskutView->setModel(proxy);
    ui->laskutView->setSelectionBehavior(QTableView::SelectRows);
    ui->laskutView->setSelectionMode(QTableView::SingleSelection);

    ui->laskutView->setModel(proxy);
    ui->laskutView->setSortingEnabled(true);
    ui->laskutView->horizontalHeader()->setStretchLastSection(true);

    connect( ui->euroSpin, SIGNAL(valueChanged(double)), this, SLOT(tarkistaKelpo()));
    connect( ui->tiliEdit, SIGNAL(textChanged(QString)), this, SLOT(tarkistaKelpo()));
    connect( ui->naytaNappi, SIGNAL(clicked()), this, SLOT(naytaLasku()));
    connect( ui->suodatusEdit, SIGNAL(textChanged(QString)), this, SLOT(suodata()));

    ui->pvmEdit->setDateRange( kp()->tilitpaatetty().addDays(1), kp()->tilikaudet()->kirjanpitoLoppuu()  );
    ui->pvmEdit->setDate( kp()->paivamaara() );

    ui->tiliEdit->suodataTyypilla("AR");

    // Valitsee oletuksena laskulle tulostuneen tilinumeron (@since 0.6)
    ui->tiliEdit->valitseTiliNumerolla( kp()->asetukset()->luku("LaskuTili") );

    connect( ui->laskutView->selectionModel(), SIGNAL(currentRowChanged(QModelIndex,QModelIndex)), this, SLOT(valintaMuuttuu()));
    connect(ui->kirjaaNappi, SIGNAL(clicked(bool)), this, SLOT(kirjaa()));

    connect( ui->tabBar, SIGNAL(currentChanged(int)), this, SLOT(valilehti()));
}

LaskunMaksuDialogi::~LaskunMaksuDialogi()
{
    delete ui;
}

void LaskunMaksuDialogi::valintaMuuttuu()
{
    QModelIndex index = ui->laskutView->currentIndex();
    ui->euroSpin->setValue( index.data(LaskutModel::AvoinnaRooli).toDouble() / 100.0 );

    // Selvitetään tällaisella vähän hassun näköisellä kyselyllä, että onko näytettävä liite olemassa

    QSqlQuery kysely( QString("SELECT id FROM liite WHERE tosite=%1 AND liiteno=%2")
                      .arg(index.data(LaskutModel::TositeRooli).toInt()).arg( index.data(LaskutModel::LiiteRooli).toInt()  ));

    ui->naytaNappi->setEnabled( kysely.next() );
}

void LaskunMaksuDialogi::kirjaa()
{
    QModelIndex index = proxy->mapToSource( ui->laskutView->currentIndex() );
    if( !index.isValid())
        return;

    QString selite = QString("%1 [%2]").arg(index.data(LaskutModel::AsiakasRooli).toString())
            .arg(index.data(LaskutModel::ViiteRooli).toString());

    bool ostolasku = ui->tabBar->currentIndex();

    VientiRivi rahaRivi;
    rahaRivi.pvm = ui->pvmEdit->date();
    rahaRivi.debetSnt = ostolasku ? 0 : qRound( ui->euroSpin->value() * 100 );
    rahaRivi.kreditSnt = ostolasku ? qRound( ui->euroSpin->value() * 100 ) : 0;
    rahaRivi.selite = selite;
    rahaRivi.tili = kp()->tilit()->tiliNumerolla( ui->tiliEdit->valittuTilinumero() );

    if( rahaRivi.tili.json()->luku("Kohdennuksella"))
        rahaRivi.kohdennus = kp()->kohdennukset()->kohdennus( index.data(LaskutModel::KohdennusIdRooli).toInt() );

    if( index.data(LaskutModel::KirjausPerusteRooli).toInt() == LaskuModel::MAKSUPERUSTE )
    {
        // Maksuperusteinen lasku kirjataan maksetuksi laskun tositteelle, joka saa
        // vasta nyt tositenumeronsa.

        KirjausWg *kwg = kirjaaja;

        int tositeNro = index.data(LaskutModel::TositeRooli).toInt() ;

        if( kwg->model()->muokattu())
        {
            // Tätä tositetta on muokattu, joten laskun maksu kirjataan uudessa ikkunassa
            LisaIkkuna* lisaIkkuna = new LisaIkkuna(nullptr);
            kwg = lisaIkkuna->kirjaa( tositeNro )->kirjausWg();
        }
        else
        {
            // Ladataan laskun tosite
            kwg->lataaTosite( tositeNro );
        }
        kwg->model()->asetaPvm( ui->pvmEdit->date() );
        kwg->model()->asetaTositelaji( kp()->asetukset()->luku("LaskuTositelaji") );
        kwg->model()->asetaTunniste( kwg->model()->seuraavaTunnistenumero() );

        for(int i=0; i < kwg->model()->vientiModel()->rowCount(QModelIndex()); i++ )
        {
            QModelIndex kirjaajaindeksi = kwg->model()->vientiModel()->index(i, VientiModel::PVM);
            if( !kirjaajaindeksi.data(VientiModel::PvmRooli).toDate().isValid() )
                kwg->model()->vientiModel()->setData( kirjaajaindeksi, ui->pvmEdit->date(), VientiModel::PvmRooli );
        }
        kwg->model()->vientiModel()->lisaaVienti(rahaRivi);

        // Lisätään erärivi, jotta saadaan erä nollatuksi
        VientiRivi eraRivi;
        eraRivi.pvm = ui->pvmEdit->date();
        eraRivi.kreditSnt = rahaRivi.debetSnt;
        eraRivi.selite = selite;
        eraRivi.tili = Tili();
        eraRivi.eraId = index.data(LaskutModel::EraIdRooli).toInt();
        eraRivi.json.set("Kirjausperuste", LaskuModel::MAKSUPERUSTE);
        kwg->model()->vientiModel()->lisaaVienti(eraRivi);
        kwg->tiedotModelista();

        laskut->maksa(index.row(), qRound(ui->euroSpin->value() * 100));

        if( kirjaaja == kwg)
            reject();       // Suljetaan laskunmaksudlg, jos ollaan aloitettu tyhjästä

    }
    else
    {
        // Tehdään saatavan vähennys
        EhdotusModel ehdotus;

        VientiRivi saatavaRivi;
        saatavaRivi.tili = kp()->tilit()->tiliIdlla( index.data(LaskutModel::TiliIdRooli).toInt() );

        saatavaRivi.kreditSnt = rahaRivi.debetSnt;
        saatavaRivi.debetSnt = rahaRivi.kreditSnt;

        saatavaRivi.eraId = index.data( LaskutModel::EraIdRooli ).toInt();
        saatavaRivi.pvm = ui->pvmEdit->date();
        saatavaRivi.selite = selite;

        if( saatavaRivi.tili.json()->luku("Kohdennuksella"))
            saatavaRivi.kohdennus = kp()->kohdennukset()->kohdennus( index.data(LaskutModel::KohdennusIdRooli).toInt() );

        ehdotus.lisaaVienti(saatavaRivi);
        ehdotus.lisaaVienti(rahaRivi);
        ehdotus.viimeisteleMaksuperusteinen();  // Tämä siksi, että maksuperusteisen lisärivit tulevat

        ehdotus.tallenna( kirjaaja->model()->vientiModel() , 0, QDate(), kirjaaja->nykyinenRivi());

        if( ostolasku)
            ostolaskut->maksa( index.row(), qRound(ui->euroSpin->value() * 100 ));
        else
            laskut->maksa(index.row(), qRound( ui->euroSpin->value() * 100));
    }
}

void LaskunMaksuDialogi::tarkistaKelpo()
{
    ui->kirjaaNappi->setEnabled( ui->euroSpin->value() > 1e-4 && ui->tiliEdit->valittuTilinumero()  );
}

void LaskunMaksuDialogi::naytaLasku()
{
    QModelIndex index =  ui->laskutView->currentIndex();

    NaytinIkkuna::naytaLiite( index.data(LaskutModel::TositeRooli).toInt(),
                           index.data(LaskutModel::LiiteRooli).toInt() );
}

void LaskunMaksuDialogi::suodata()
{
    QString suodatin = ui->suodatusEdit->text();
    if( suodatin.toInt())
        proxy->setFilterKeyColumn( LaskutModel::NUMERO);
    else
        proxy->setFilterKeyColumn( LaskutModel::ASIAKAS);
    proxy->setFilterFixedString( suodatin );
}

void LaskunMaksuDialogi::valilehti()
{
    if( ui->tabBar->currentIndex())
    {
        proxy->setSourceModel( ostolaskut );
    }
    else
    {
        proxy->setSourceModel( laskut);
    }
}
