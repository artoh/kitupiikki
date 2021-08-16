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
#include <QApplication>

#include <QDebug>
#include <QScrollBar>
#include <QRegularExpressionValidator>
#include <QMessageBox>

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
    proxy->setFilterCaseSensitivity(Qt::CaseInsensitive);
    proxy->setSortRole(TiliModel::LajitteluRooli);
    proxy->sort(0);

    ui->view->setModel(proxy);
    ui->view->hideColumn( TiliModel::NRONIMI);
    ui->view->horizontalHeader()->setSectionResizeMode( TiliModel::NIMI, QHeaderView::Stretch );

    ui->siirryEdit->setValidator(new QRegularExpressionValidator(QRegularExpression("[1-9][0-9]{0,7}")));


    connect( ui->kaikkiNappi, &QPushButton::clicked, this, [this]() { this->suodataTila(0);}  );
    connect( ui->kaytossaNappi, &QPushButton::clicked, this, [this]() {this->suodataTila(1);});
    connect( ui->suosikitNappi, &QPushButton::clicked, this, [this]() { this->suodataTila(2); });

    connect( ui->piilotaNappi, &QPushButton::clicked, this, [this] () { this->muutaTila(Tili::TILI_PIILOSSA); } );
    connect( ui->naytaNappi, &QPushButton::clicked, this, [this] () { this->muutaTila(Tili::TILI_KAYTOSSA); } );
    connect( ui->suosikkiNappi, &QPushButton::clicked, this, [this] () { this->muutaTila(Tili::TILI_SUOSIKKI); } );
    connect( ui->normaaliNappi, &QPushButton::clicked, this, [this] () { this->muutaTila(Tili::TILI_NORMAALI); } );

    connect(ui->view->selectionModel(), SIGNAL(currentRowChanged(QModelIndex,QModelIndex)),
            this, SLOT(riviValittu(QModelIndex)));

    connect(ui->muokkaaNappi, SIGNAL(clicked(bool)), this, SLOT(muokkaa()));

    connect( ui->uusiTiliNappi, &QPushButton::clicked, this, &TilikarttaMuokkaus::uusiTili);
    connect( ui->uusiOtsikkoNappi, &QPushButton::clicked, this, &TilikarttaMuokkaus::uusiOtsikko);


    connect( ui->view, SIGNAL(doubleClicked(QModelIndex)), this, SLOT(muokkaa()));
    connect( ui->suodataEdit, SIGNAL(textChanged(QString)), this, SLOT(suodata(QString)));
    connect(ui->siirryEdit, SIGNAL(textChanged(QString)),this, SLOT(siirry(QString)));

    suodataTila(1);
    riviValittu(QModelIndex());
}

TilikarttaMuokkaus::~TilikarttaMuokkaus()
{
    delete ui;
}

bool TilikarttaMuokkaus::nollaa()
{
    ui->view->horizontalHeader()->setSectionResizeMode(2, QHeaderView::Stretch);
    ui->view->selectRow(0);
    model->haeSaldot();
    return true;
}



void TilikarttaMuokkaus::muutaTila(Tili::TiliTila tila)
{
    int indeksi = naytaProxy->mapToSource( proxy->mapToSource(ui->view->currentIndex())).row();
    Tili* tili = kp()->tilit()->tiliPIndeksilla(indeksi);

    if( tili->otsikkotaso())  {
        // Muutetaan kaikki tilit tämän alapuolelta
        for(int i=indeksi+1; i < kp()->tilit()->rowCount(); i++) {
            Tili* tamatili = kp()->tilit()->tiliPIndeksilla(i);
            if( !tamatili->otsikkotaso())
                kp()->tilit()->asetaSuosio( tamatili->numero(), tila );
            else if( tamatili->otsikkotaso() <= tili->otsikkotaso())
                break;
        }
    } else {
        kp()->tilit()->asetaSuosio( tili->numero(), tila);
    }

    riviValittu(ui->view->currentIndex());

}

void TilikarttaMuokkaus::riviValittu(const QModelIndex& index)
{
    ui->muokkaaNappi->setEnabled( index.isValid());
    ui->view->scrollTo(ui->view->selectionModel()->selection().indexes().value(0));


    if( index.isValid() ) {

        bool piilotettavissa = index.data(TiliModel::OtsikkotasoRooli).toInt() != 1;

        ui->piilotaNappi->setEnabled(piilotettavissa);
        ui->normaaliNappi->setEnabled(true);
        ui->naytaNappi->setEnabled(true);
        ui->suosikkiNappi->setEnabled(piilotettavissa);

        int tila = index.data(TiliModel::TilaValintaRooli).toInt();

        ui->piilotaNappi->setChecked( tila == Tili::TILI_PIILOSSA);
        ui->normaaliNappi->setChecked( tila == Tili::TILI_NORMAALI);
        ui->naytaNappi->setChecked( tila == Tili::TILI_KAYTOSSA);
        ui->suosikkiNappi->setChecked( tila == Tili::TILI_SUOSIKKI);
    } else {
        ui->piilotaNappi->setEnabled(false);
        ui->normaaliNappi->setEnabled(false);
        ui->naytaNappi->setEnabled(false);
        ui->suosikkiNappi->setEnabled(false);
    }



}

void TilikarttaMuokkaus::muokkaa()
{
    TilinMuokkausDialog dlg(this, naytaProxy->mapToSource( proxy->mapToSource(ui->view->currentIndex())).row(), TilinMuokkausDialog::MUOKKAA );
    dlg.exec();
}

void TilikarttaMuokkaus::uusiTili()
{
    suodataTila(0);
    ui->view->scrollTo(ui->view->currentIndex(), QAbstractItemView::PositionAtCenter);

    qApp->processEvents();
    TilinMuokkausDialog dlg(this, naytaProxy->mapToSource( proxy->mapToSource(ui->view->currentIndex())).row(), TilinMuokkausDialog::UUSITILI );

    dlg.exec();
    proxy->sort(0);
}

void TilikarttaMuokkaus::uusiOtsikko()
{
    TilinMuokkausDialog dlg(this, naytaProxy->mapToSource( proxy->mapToSource(ui->view->currentIndex())).row(), TilinMuokkausDialog::UUSIOTSIKKO );
    dlg.exec();
    proxy->sort(0);
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

