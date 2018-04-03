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

    laskut = new LaskutModel(this);
    laskut->lataaAvoimet();

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

    ui->pvmEdit->setDateRange( kp()->tilikausiPaivalle( kp()->paivamaara()).alkaa(),
                               kp()->tilikausiPaivalle( kp()->paivamaara()).paattyy());
    ui->pvmEdit->setDate( kp()->paivamaara() );

    ui->tiliEdit->suodataTyypilla("AR");

    // Valitsee oletuksena laskulle tulostuneen tilinumeron (@since 0.6)
    ui->tiliEdit->valitseTiliNumerolla( kp()->asetukset()->luku("LaskuTili") );

    connect( ui->laskutView->selectionModel(), SIGNAL(currentRowChanged(QModelIndex,QModelIndex)), this, SLOT(valintaMuuttuu()));
    connect(ui->kirjaaNappi, SIGNAL(clicked(bool)), this, SLOT(kirjaa()));
}

LaskunMaksuDialogi::~LaskunMaksuDialogi()
{
    delete ui;
}

void LaskunMaksuDialogi::valintaMuuttuu()
{
    QModelIndex index = ui->laskutView->currentIndex();
    ui->euroSpin->setValue( index.data(LaskutModel::AvoinnaRooli).toDouble() / 100.0 );
}

void LaskunMaksuDialogi::kirjaa()
{
    QModelIndex index = proxy->mapToSource( ui->laskutView->currentIndex() );
    if( !index.isValid())
        return;

    JsonKentta json( index.data(LaskutModel::JsonRooli).toByteArray() );

    QString selite = QString("%1 [%2]").arg(index.data(LaskutModel::AsiakasRooli).toString())
            .arg(index.data(LaskutModel::ViiteRooli).toInt());

    VientiRivi rahaRivi;
    rahaRivi.pvm = ui->pvmEdit->date();
    rahaRivi.debetSnt = int( ui->euroSpin->value() * 100 );
    rahaRivi.selite = selite;
    rahaRivi.tili = kp()->tilit()->tiliNumerolla( ui->tiliEdit->valittuTilinumero() );
    rahaRivi.maksaaLaskua = index.data(LaskutModel::ViiteRooli).toInt();
    rahaRivi.kohdennus = kp()->kohdennukset()->kohdennus( json.luku("Kohdennus") );

    if( index.data(LaskutModel::KirjausPerusteRooli).toInt() == LaskuModel::MAKSUPERUSTE )
    {
        if( kirjaaja->model()->muokattu())
        {
            if( QMessageBox::question(this, tr("Hylkää nykyinen kirjaus"),
                                      tr("Maksuperusteisen laskun kirjaus tehdään uuteen tositteeseen. "
                                         "Hylkäätkö nykyisen tositteen tallentamatta tekemiäsi muutoksia?")) != QMessageBox::Yes)
                return;
        }
        // Ladataan laskun tosite
        kirjaaja->lataaTosite( index.data(LaskutModel::TositeRooli).toInt() );
        kirjaaja->model()->asetaPvm( ui->pvmEdit->date() );
        kirjaaja->model()->asetaTositelaji( kp()->asetukset()->luku("LaskuTositelaji") );

        for(int i=0; i < kirjaaja->model()->vientiModel()->rowCount(QModelIndex()); i++ )
        {
            kirjaaja->model()->vientiModel()->setData( kirjaaja->model()->vientiModel()->index(i, VientiModel::PVM), ui->pvmEdit->date(), VientiModel::PvmRooli );
        }
        kirjaaja->model()->vientiModel()->lisaaVienti(rahaRivi);
        laskut->maksa(index.row(), ui->euroSpin->value() * 100);
        reject();

    }
    else
    {
        // Tehdään saatavan vähennys
        EhdotusModel ehdotus;

        VientiRivi saatavaRivi;
        saatavaRivi.tili = kp()->tilit()->tiliNumerolla( json.luku("Saatavatili") );
        saatavaRivi.kreditSnt = qlonglong( ui->euroSpin->value() * 100 );
        saatavaRivi.eraId = json.luku("TaseEra");
        saatavaRivi.pvm = ui->pvmEdit->date();
        saatavaRivi.selite = selite;
        saatavaRivi.kohdennus = kp()->kohdennukset()->kohdennus(json.luku("Kohdennus"));

        ehdotus.lisaaVienti(saatavaRivi);
        ehdotus.lisaaVienti(rahaRivi);
        ehdotus.viimeisteleMaksuperusteinen();  // Tämä siksi, että maksuperusteisen lisärivit tulevat

        ehdotus.tallenna( kirjaaja->model()->vientiModel() );

        laskut->maksa(index.row(), ui->euroSpin->value() * 100);
    }


}

void LaskunMaksuDialogi::tarkistaKelpo()
{
    ui->kirjaaNappi->setEnabled( ui->euroSpin->value() && ui->tiliEdit->valittuTilinumero()  );
}

void LaskunMaksuDialogi::naytaLasku()
{
    QModelIndex index =  ui->laskutView->currentIndex();
    QString liite = index.data(LaskutModel::LiiteRooli).toString();
    if(!liite.isEmpty())
    {
        Kirjanpito::avaaUrl( QUrl( LiiteModel::liiteNimella(liite) ) );
    }
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
