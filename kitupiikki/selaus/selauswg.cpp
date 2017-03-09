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

#include "selauswg.h"
#include "db/kirjanpito.h"
#include "selausmodel.h"
#include <QDate>
#include <QSortFilterProxyModel>
#include <QSqlQuery>

#include <QDebug>

#include "tositeselausmodel.h"

SelausWg::SelausWg() :
    KitupiikkiSivu(),
    ui(new Ui::SelausWg)
{
    ui->setupUi(this);
    model = new SelausModel();
    tositeModel = new TositeSelausModel();

    proxyModel = new QSortFilterProxyModel(this);
    proxyModel->setSourceModel(model);
    proxyModel->setSortRole(Qt::EditRole);  // Jotta numerot lajitellaan oikein
    proxyModel->setFilterKeyColumn( SelausModel::TILI);

    etsiProxy = new QSortFilterProxyModel(this);
    etsiProxy->setSourceModel(proxyModel);
    etsiProxy->setFilterKeyColumn( SelausModel::SELITE );
    ui->selausView->setModel( etsiProxy );

    ui->selausView->horizontalHeader()->setStretchLastSection(true);
    ui->selausView->verticalHeader()->hide();

    ui->selausView->sortByColumn(SelausModel::PVM, Qt::AscendingOrder);

    connect( ui->etsiEdit, SIGNAL(textChanged(QString)), etsiProxy, SLOT(setFilterFixedString(QString)));

    connect( ui->alkuEdit, SIGNAL(editingFinished()), this, SLOT(paivita()));
    connect( ui->loppuEdit, SIGNAL(editingFinished()), this, SLOT(paivita()));
    connect( ui->tiliCombo, SIGNAL(currentTextChanged(QString)), this, SLOT(suodata()));
    connect( ui->selausView, SIGNAL(activated(QModelIndex)), this, SLOT(naytaTositeRivilta(QModelIndex)));

    connect( ui->tosittetBtn, SIGNAL(clicked(bool)), this, SLOT(selaaTositteita()));
    connect( ui->viennitBtn, SIGNAL(clicked(bool)), this, SLOT(selaaVienteja()));

    connect( Kirjanpito::db(), SIGNAL(kirjanpitoaMuokattu()), this, SLOT(merkitsePaivitettavaksi()));
    connect( kp(), SIGNAL(tietokantaVaihtui()), this, SLOT(alusta()));


}

SelausWg::~SelausWg()
{
    delete ui;
}

void SelausWg::alusta()
{
    QDate alku = Kirjanpito::db()->tilikaudet()->kirjanpitoAlkaa();

    Tilikausi nytkausi = Kirjanpito::db()->tilikaudet()->tilikausiPaivalle( Kirjanpito::db()->paivamaara() );

    QDate nytalkaa = nytkausi.alkaa();
    QDate nytloppuu = nytkausi.paattyy();
    QDate loppu = Kirjanpito::db()->tilikaudet()->kirjanpitoLoppuu();

    ui->alkuEdit->setDateRange(alku, loppu);
    ui->loppuEdit->setDateRange(alku, loppu);
    ui->alkuEdit->setDate(nytalkaa);
    ui->loppuEdit->setDate(nytloppuu);
}

void SelausWg::paivita()
{
    if( ui->viennitBtn->isChecked())
    {
        model->lataa( ui->alkuEdit->date(), ui->loppuEdit->date());

        QString valittu = ui->tiliCombo->currentText();
        ui->tiliCombo->clear();
        ui->tiliCombo->insertItem(0, QIcon(":/pic/Possu64.png"),"Kaikki tilit", QVariant("*"));
        ui->tiliCombo->insertItems(1, model->kaytetytTilit());
        ui->tiliCombo->setCurrentText(valittu);

    }
    else
    {
        tositeModel->lataa( ui->alkuEdit->date(), ui->loppuEdit->date());

        QString valittu = ui->tiliCombo->currentText();
        ui->tiliCombo->clear();
        ui->tiliCombo->insertItem(0, QIcon(":/pic/Possu64.png"),"Kaikki tositteet", QVariant("*"));
        ui->tiliCombo->insertItems(1, tositeModel->lajiLista() );
        ui->tiliCombo->setCurrentText(valittu);

    }

    ui->selausView->resizeColumnsToContents();
    paivitaSummat();
    paivitettava = false;


}

void SelausWg::suodata()
{
    if( ui->tiliCombo->currentData().toString() == "*")
        proxyModel->setFilterFixedString(QString());
    else
        proxyModel->setFilterFixedString( ui->tiliCombo->currentText());
    paivitaSummat();
}

void SelausWg::paivitaSummat()
{
    if( ui->tosittetBtn->isChecked())
    {
        // Summia ei näytetä tositelistalle ;)
        ui->summaLabel->clear();
        return;
    }

    int debetSumma = 0;
    int kreditSumma = 0;

    for(int i=0; i<proxyModel->rowCount(QModelIndex()); i++)
    {
        debetSumma += proxyModel->index(i, SelausModel::DEBET).data(Qt::EditRole).toInt();
        kreditSumma += proxyModel->index(i, SelausModel::KREDIT).data(Qt::EditRole).toInt();
    }

    QString teksti = tr("Debet %L1 €  Kredit %L2 €").arg( ((double)debetSumma)/100.0 ,0,'f',2)
            .arg(((double)kreditSumma) / 100.0 ,0,'f',2);

    if( ui->tiliCombo->currentData().isNull())
    {
        // Tili on valittuna
        QString valittuTekstina = ui->tiliCombo->currentText();
        int valittunro = valittuTekstina.left( valittuTekstina.indexOf(' ') ).toInt();
        Tili valittutili = Kirjanpito::db()->tilit()->tiliNumerolla(valittunro);

        int saldo = valittutili.saldoPaivalle( ui->loppuEdit->date());
        int muutos = kreditSumma - debetSumma;

        if( valittutili.onkoVastaavaaTili() )
        {
            muutos = debetSumma - kreditSumma;
        }


        teksti += tr("\nMuutos %L2€ Loppusaldo %L1 €").arg( ((double) saldo ) / 100.0, 0, 'f', 2)
                .arg(((double)muutos) / 100.0, 0, 'f', 2  );
    }
    ui->summaLabel->setText(teksti);
}

void SelausWg::naytaTositeRivilta(QModelIndex index)
{
    int id = index.data( Qt::UserRole).toInt();
    emit tositeValittu( id );
}

void SelausWg::selaa(int tilinumero, Tilikausi tilikausi)
{
    // Ohjelmallisesti selaa tiettynä tilikautena tiettyä tiliä
    ui->alkuEdit->setDate( tilikausi.alkaa());
    ui->loppuEdit->setDate( tilikausi.paattyy());

    if( !ui->viennitBtn->isChecked())
         ui->viennitBtn->click();    // Valitaan viennit-näkymä

    paivita();

    Tili selattava = Kirjanpito::db()->tilit()->tiliNumerolla(tilinumero);

    ui->tiliCombo->setCurrentText(QString("%1 %2").arg(selattava.numero() ).arg(selattava.nimi()));

}

void SelausWg::selaaVienteja()
{
    ui->tosittetBtn->setChecked(false);
    ui->viennitBtn->setChecked(true);

    proxyModel->setSourceModel(model);
    proxyModel->setFilterKeyColumn( SelausModel::TILI);
    proxyModel->setSortRole(Qt::EditRole);  // Jotta numerot lajitellaan oikein
    etsiProxy->setSourceModel(proxyModel);
    etsiProxy->setFilterKeyColumn( SelausModel::SELITE );

    paivita();
}

void SelausWg::selaaTositteita()
{
    ui->viennitBtn->setChecked(false);
    ui->tosittetBtn->setChecked(true);

    proxyModel->setSourceModel(tositeModel);
    proxyModel->setFilterKeyColumn( TositeSelausModel::TOSITELAJI);
    proxyModel->setSortRole(Qt::EditRole);  // Jotta numerot lajitellaan oikein
    etsiProxy->setSourceModel(proxyModel);
    etsiProxy->setFilterKeyColumn( SelausModel::SELITE );

    paivita();
}

void SelausWg::siirrySivulle()
{
    // Sivu päivitetään, jos jotain on muuttunut
    if( paivitettava )
        paivita();
}

void SelausWg::merkitsePaivitettavaksi()
{
    paivitettava = true;
}
