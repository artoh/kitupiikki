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
#include <QScrollBar>

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

    naytaProxy = new QSortFilterProxyModel(this);
    naytaProxy->setSourceModel(model);
    naytaProxy->setFilterRole(TiliModel::TilaRooli);

    proxy = new QSortFilterProxyModel(this);
    proxy->setSourceModel(naytaProxy);
    proxy->setSortRole(TiliModel::YsiRooli);
    proxy->setFilterCaseSensitivity(Qt::CaseInsensitive);

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

    connect( ui->kaikkiNappi, &QPushButton::clicked, [this]() { this->suodataTila(0);}  );
    connect( ui->kaytossaNappi, &QPushButton::clicked, [this]() {this->suodataTila(1);});
    connect( ui->suosikitNappi, &QPushButton::clicked, [this]() { this->suodataTila(2); });

    connect(ui->view->selectionModel(), SIGNAL(currentRowChanged(QModelIndex,QModelIndex)),
            this, SLOT(riviValittu(QModelIndex)));

    connect(ui->muokkaaNappi, SIGNAL(clicked(bool)), this, SLOT(muokkaa()));
    connect( ui->uusiNappi, SIGNAL(clicked(bool)), this, SLOT(uusi()));
    connect( ui->poistaNappi, SIGNAL(clicked(bool)), this, SLOT(poista()));

    connect( ui->view, SIGNAL(doubleClicked(QModelIndex)), this, SLOT(muokkaa()));
    connect( ui->suodataEdit, SIGNAL(textChanged(QString)), this, SLOT(suodata(QString)));
    connect(ui->siirryEdit, SIGNAL(textChanged(QString)),this, SLOT(siirry(QString)));

}

TilikarttaMuokkaus::~TilikarttaMuokkaus()
{
    delete ui;
}

bool TilikarttaMuokkaus::nollaa()
{
    model->lataa();
    proxy->sort(0);

    ui->view->resizeColumnsToContents();
    ui->view->horizontalHeader()->stretchLastSection();
    return true;
}

bool TilikarttaMuokkaus::tallenna()
{
    model->tallenna();
    kp()->tilit()->lataa();
    kp()->onni(tr("Tilikartta tallennettu"));
    return true;
}

bool TilikarttaMuokkaus::onkoMuokattu()
{
    return model->onkoMuokattu();
}

void TilikarttaMuokkaus::muutaTila(int tila)
{
    QModelIndex index = ui->view->currentIndex();
    if( index.isValid())
    {
        proxy->setData(index, tila, TiliModel::TilaRooli);
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
                proxy->setData( proxy->index(r, 0) , tila, TiliModel::TilaRooli );
            }
        }
    }
    riviValittu(index);
    tallennaKaytossa( onkoMuokattu() );
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

    TilinMuokkausDialog dlg(model, naytaProxy->mapToSource( proxy->mapToSource(ui->view->currentIndex())));
    dlg.exec();
    proxy->sort(0);
    emit tallennaKaytossa( onkoMuokattu() );

}

void TilikarttaMuokkaus::uusi()
{
    TilinMuokkausDialog dlg(model);
    dlg.exec();
    proxy->sort(0);
    emit tallennaKaytossa( onkoMuokattu() );
}

void TilikarttaMuokkaus::poista()
{
    if( ui->view->currentIndex().isValid())
        model->poistaRivi(  proxy->mapToSource(ui->view->currentIndex()).row());
    emit tallennaKaytossa( onkoMuokattu() );
}

void TilikarttaMuokkaus::suodataTila(int tila)
{
    ui->kaikkiNappi->setChecked(tila == 0);
    ui->kaytossaNappi->setChecked(tila == 1);
    ui->suosikitNappi->setChecked(tila == 2);

    switch (tila) {
    case 0:
        naytaProxy->setFilterFixedString("");
        break;
    case 1:
        naytaProxy->setFilterRegExp("(1|2)");
        break;
    case 2:
        naytaProxy->setFilterFixedString("2");
    }
}

void TilikarttaMuokkaus::suodata(const QString &teksti)
{
    if( teksti.toInt())
        proxy->setFilterKeyColumn(TiliModel::NUMERO);
    else
        proxy->setFilterKeyColumn(TiliModel::NIMI);
    proxy->setFilterFixedString(teksti);
}

void TilikarttaMuokkaus::siirry(const QString &minne)
{
    if( !minne.isEmpty())
    {
        for(int i=0; i < ui->view->model()->rowCount(QModelIndex()); i++)
        {
            QModelIndex index = ui->view->model()->index(i,0);

            if( QString::number(index.data(TiliModel::NroRooli).toInt()).startsWith(minne) ||
                index.data(TiliModel::NimiRooli).toString().startsWith(minne))
            {
                // Valitsee tämän rivin
                ui->view->setCurrentIndex(index);
                // Ja scrollaa rivin näkyviin
                ui->view->verticalScrollBar()->setSliderPosition(i);
                return;
            }
        }
    }
}

