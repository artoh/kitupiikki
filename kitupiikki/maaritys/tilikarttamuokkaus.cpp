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
#include <QRegularExpressionValidator>

#include "tilikarttamuokkaus.h"
#include "db/kirjanpito.h"
#include "db/tili.h"
#include "tilinmuokkausdialog.h"


TilikarttaMuokkaus::TilikarttaMuokkaus(QWidget *parent)
    : MaaritysWidget(parent)
{
    ui = new Ui::Tilikartta;
    ui->setupUi(this);


    model = kp()->tilit();

    naytaProxy = new QSortFilterProxyModel(this);
    naytaProxy->setSourceModel(model);
    naytaProxy->setFilterRole(TiliModel::TilaRooli);

    proxy = new QSortFilterProxyModel(this);
    proxy->setSourceModel(naytaProxy);
    proxy->setSortRole(TiliModel::YsiRooli);
    proxy->setFilterCaseSensitivity(Qt::CaseInsensitive);

    ui->view->setModel(proxy);
    ui->view->hideColumn( TiliModel::NRONIMI);
    ui->view->horizontalHeader()->setSectionResizeMode( TiliModel::NIMI, QHeaderView::Stretch );

    ui->siirryEdit->setValidator(new QRegularExpressionValidator(QRegularExpression("[1-9][0-9]{0,7}")));


    connect( ui->kaikkiNappi, &QPushButton::clicked, [this]() { this->suodataTila(0);}  );
    connect( ui->kaytossaNappi, &QPushButton::clicked, [this]() {this->suodataTila(1);});
    connect( ui->suosikitNappi, &QPushButton::clicked, [this]() { this->suodataTila(2); });

    connect(ui->view->selectionModel(), SIGNAL(currentRowChanged(QModelIndex,QModelIndex)),
            this, SLOT(riviValittu(QModelIndex)));

    connect(ui->muokkaaNappi, SIGNAL(clicked(bool)), this, SLOT(muokkaa()));
    connect( ui->poistaNappi, SIGNAL(clicked(bool)), this, SLOT(poista()));

    connect( ui->uusiTiliNappi, &QPushButton::clicked, this, &TilikarttaMuokkaus::uusiTili);


    connect( ui->view, SIGNAL(doubleClicked(QModelIndex)), this, SLOT(muokkaa()));
    connect( ui->suodataEdit, SIGNAL(textChanged(QString)), this, SLOT(suodata(QString)));
    connect(ui->siirryEdit, SIGNAL(textChanged(QString)),this, SLOT(siirry(QString)));

    suodataTila(1);
}

TilikarttaMuokkaus::~TilikarttaMuokkaus()
{
    delete ui;
}

bool TilikarttaMuokkaus::nollaa()
{
    proxy->sort(0);

    ui->view->resizeColumnsToContents();
    ui->view->horizontalHeader()->stretchLastSection();
    ui->view->selectRow(0);
    return true;
}

bool TilikarttaMuokkaus::tallenna()
{
    kp()->onni(tr("Tilikartta tallennettu"));
    return true;
}

bool TilikarttaMuokkaus::onkoMuokattu()
{
    return model->onkoMuokattu();
}

void TilikarttaMuokkaus::muutaTila(int tila)
{
    QModelIndex index = naytaProxy->mapToSource(  proxy->mapToSource( ui->view->currentIndex() ) );
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

        if( tila )
        {
            // Jos tila muutettu näkyväksi, niin muutetaan myös otsikot tästä ylöspäin
            int edtaso = index.data(TiliModel::OtsikkotasoRooli).toInt() ? index.data(TiliModel::OtsikkotasoRooli).toInt() : 10;            
            bool muuttaa = true;

            for(int r = index.row()-1; r > -1; r--)
            {
                int otsikkotaso = model->index(r,0).data(TiliModel::OtsikkotasoRooli).toInt();

                if( otsikkotaso >= edtaso )
                    muuttaa = false;
                else if(otsikkotaso)
                    muuttaa = true;

                if( otsikkotaso && muuttaa )
                {
                    edtaso = otsikkotaso;
                    if( model->index(r,0).data(TiliModel::TilaRooli).toInt() == 0)
                        model->setData( model->index(r,0), 1, TiliModel::TilaRooli );
                }

                if( otsikkotaso == 1)
                    break;
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

}

void TilikarttaMuokkaus::muokkaa()
{


}

void TilikarttaMuokkaus::uusiTili()
{
    TilinMuokkausDialog dlg(this, naytaProxy->mapToSource( proxy->mapToSource(ui->view->currentIndex())).row(), TilinMuokkausDialog::UUSITILI );
    dlg.exec();
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

