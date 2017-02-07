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

SelausWg::SelausWg() :
    QWidget(),
    ui(new Ui::SelausWg)
{
    ui->setupUi(this);
    model = new SelausModel();

    proxyModel = new QSortFilterProxyModel(this);
    proxyModel->setSourceModel(model);
    proxyModel->setSortRole(Qt::EditRole);  // Jotta numerot lajitellaan oikein
    proxyModel->setFilterKeyColumn( SelausModel::TILI);

    ui->selausView->setModel(proxyModel);

    ui->selausView->horizontalHeader()->setStretchLastSection(true);
    ui->selausView->verticalHeader()->hide();

    ui->selausView->sortByColumn(SelausModel::PVM, Qt::AscendingOrder);

    connect( ui->alkuEdit, SIGNAL(editingFinished()), this, SLOT(paivita()));
    connect( ui->loppuEdit, SIGNAL(editingFinished()), this, SLOT(paivita()));
    connect( ui->tiliCombo, SIGNAL(currentTextChanged(QString)), this, SLOT(suodata()));

    connect( ui->selausView, SIGNAL(activated(QModelIndex)), this, SLOT(naytaTositeRivilta(QModelIndex)));

    connect( Kirjanpito::db(), SIGNAL(kirjanpitoaMuokattu()), this, SLOT(paivita()));

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

    paivita();
}

void SelausWg::paivita()
{
    model->lataa( ui->alkuEdit->date(), ui->loppuEdit->date());
    ui->selausView->resizeColumnsToContents();

    QString valittu = ui->tiliCombo->currentText();
    ui->tiliCombo->clear();
    ui->tiliCombo->insertItem(0, QIcon(":/pic/Possu64.png"),"Kaikki tilit", QVariant("*"));
    ui->tiliCombo->insertItems(1, model->kaytetytTilit());
    ui->tiliCombo->setCurrentText(valittu);
    paivitaSummat();
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

        int kertyma = 0;
        int muutos = 0;
        if( valittutili.onkoTasetili())
        {
            kertyma = valittutili.kertymaPaivalle( ui->loppuEdit->date());
            muutos = debetSumma - kreditSumma;
        }
        else
        {
            // Tulostili
            // Tämän tilikauden aloittavaa päivää edeltävä päivä, jonka kertymä vähennetään
            QDate edloppuupvm = Kirjanpito::db()->tilikausiPaivalle(ui->loppuEdit->date()).alkaa().addDays(-1);
            kertyma = valittutili.kertymaPaivalle( ui->loppuEdit->date()) - valittutili.kertymaPaivalle( edloppuupvm );
            muutos = kreditSumma - debetSumma;
        }

        teksti += tr("\nMuutos %L2€ Loppusaldo %L1 €").arg( ((double) kertyma ) / 100.0, 0, 'f', 2)
                .arg(((double)muutos) / 100.0, 0, 'f', 2  );
    }
    ui->summaLabel->setText(teksti);
}

void SelausWg::naytaTositeRivilta(QModelIndex index)
{
    int id = index.data( Qt::UserRole).toInt();
    qDebug() << "Näyttöpyyntö " << id;
    emit tositeValittu( id );
}

void SelausWg::selaa(int tilinumero, Tilikausi tilikausi)
{
    // Ohjelmallisesti selaa tiettynä tilikautena tiettyä tiliä
    ui->alkuEdit->setDate( tilikausi.alkaa());
    ui->loppuEdit->setDate( tilikausi.paattyy());
    paivita();

    Tili selattava = Kirjanpito::db()->tilit()->tiliNumerolla(tilinumero);

    ui->tiliCombo->setCurrentText(QString("%1 %2").arg(selattava.numero() ).arg(selattava.nimi()));

}
