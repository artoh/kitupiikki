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

#include <QSignalMapper>

#include <QDebug>

#include "tilikarttamuokkaus.h"
#include "db/kirjanpito.h"
#include "db/tili.h"
#include "tilinmuokkausdialog.h"

TilikarttaMuokkaus::TilikarttaMuokkaus(QWidget *parent)
    : MaaritysWidget(parent)
{
    ui = new Ui::Tilikartta;
    ui->setupUi(this);


    model = new TiliModel( kp()->tietokanta(), this);
    proxy = new QSortFilterProxyModel(this);
    proxy->setSourceModel(model);
    proxy->setSortRole(TiliModel::YsiRooli);

    ui->view->setModel(proxy);
    ui->view->hideColumn( TiliModel::NRONIMI);

    // Tilin tilaa muuttavat napit käsitellään signaalimappauksella
    QSignalMapper *tilamapper = new QSignalMapper(this);
    connect(ui->piilotaNappi, SIGNAL(clicked()), tilamapper, SLOT(map()));
    tilamapper->setMapping(ui->piilotaNappi, 0);
    connect(ui->normaaliNappi, SIGNAL(clicked()), tilamapper, SLOT(map()));
    tilamapper->setMapping(ui->normaaliNappi, 1);
    connect(ui->suosikkiNappi, SIGNAL(clicked()), tilamapper, SLOT(map()));
    tilamapper->setMapping(ui->suosikkiNappi, 2);
    connect(tilamapper, SIGNAL(mapped(int)), this, SLOT(muutaTila(int)));

    connect(ui->view->selectionModel(), SIGNAL(currentRowChanged(QModelIndex,QModelIndex)),
            this, SLOT(riviValittu(QModelIndex)));

    connect(ui->muokkaaNappi, SIGNAL(clicked(bool)), this, SLOT(muokkaa()));
    connect( ui->uusiNappi, SIGNAL(clicked(bool)), this, SLOT(uusi()));
    connect( ui->view, SIGNAL(doubleClicked(QModelIndex)), this, SLOT(muokkaa()));

}

TilikarttaMuokkaus::~TilikarttaMuokkaus()
{
    delete ui;
}

bool TilikarttaMuokkaus::nollaa()
{
    model->lataa();
    ui->view->resizeColumnsToContents();
    ui->view->horizontalHeader()->stretchLastSection();
    return true;
}

bool TilikarttaMuokkaus::tallenna()
{
    model->tallenna();
    kp()->tilit()->lataa();
    return true;
}

bool TilikarttaMuokkaus::onkoMuokattu()
{
    return false;
}

void TilikarttaMuokkaus::muutaTila(int tila)
{
    QModelIndex index = ui->view->currentIndex();
    if( index.isValid())
    {
        model->setData(index, tila, TiliModel::TilaRooli);
        if( index.data(TiliModel::OtsikkotasoRooli).toInt())
        {
            // Otsikkotaso - tila muutetaan kaikkialle alle
            int tamanotsikkotaso = index.data(TiliModel::OtsikkotasoRooli).toInt();
            for( int r = index.row() + 1; r < model->rowCount(QModelIndex()); r++)
            {
                int otsikkotaso = model->index(r,0).data(TiliModel::OtsikkotasoRooli).toInt();
                // Pitää olla otsikko, mutta taso enintään verrokki
                // Eli jos muutetaan H2 niin myös H3,H4... muuttuu
                if( (otsikkotaso > 0) && (otsikkotaso <= tamanotsikkotaso ))
                    break;      // Löydetty seuraava samantasoinen otsikko
                model->setData( model->index(r, 0) , tila, TiliModel::TilaRooli );
            }
        }
    }
    riviValittu(index);
}

void TilikarttaMuokkaus::riviValittu(const QModelIndex& index)
{
    ui->muokkaaNappi->setEnabled( index.isValid());


    int tila = index.data(TiliModel::TilaRooli).toInt();
    ui->piilotaNappi->setChecked( tila == 0);
    ui->normaaliNappi->setChecked( tila == 1);
    ui->suosikkiNappi->setChecked( tila == 2);

    Tili tili = model->tiliIndeksilla( index.row());
    ui->poistaNappi->setDisabled( tili.montakoVientia() );
}

void TilikarttaMuokkaus::muokkaa()
{

    TilinMuokkausDialog dlg(model, proxy->mapToSource(ui->view->currentIndex()));
    dlg.exec();
    proxy->sort(0);

}

void TilikarttaMuokkaus::uusi()
{
    TilinMuokkausDialog dlg(model);
    dlg.exec();
    proxy->sort(0);
}

