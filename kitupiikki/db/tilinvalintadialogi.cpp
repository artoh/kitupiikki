
/*
   Copyright (C) 2017,2018 Arto Hyvättinen

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

#include <QDebug>

#include "tilinvalintadialogi.h"
#include "ui_tilinvalintadialogi.h"

#include "kirjanpito.h"

TilinValintaDialogi::TilinValintaDialogi(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::TilinValintaDialogi)
{
    ui->setupUi(this);

    proxyNimi = new QSortFilterProxyModel(this);
    asetaModel( kp()->tilit());
    proxyNimi->setFilterRole( TiliModel::NimiRooli);
    proxyNimi->setFilterCaseSensitivity(Qt::CaseInsensitive);
    proxyNimi->setSortRole(TiliModel::YsiRooli);

    proxyTyyppi = new QSortFilterProxyModel(this);
    proxyTyyppi->setSourceModel(proxyNimi);
    proxyTyyppi->setFilterRole(TiliModel::TyyppiRooli);

    proxyTila = new QSortFilterProxyModel(this);
    proxyTila->setSourceModel(proxyTyyppi);
    proxyTila->setFilterRole(TiliModel::TilaRooli);
    proxyTila->setFilterRegExp("[12]");

    ui->view->setModel(proxyTila);
    ui->view->setColumnHidden(TiliModel::NRONIMI, true);

    ui->view->resizeColumnsToContents();

    ui->infoMerkki->setVisible(false);
    ui->ohjeLabel->setVisible(false);

    connect( ui->suosikitNappi, SIGNAL(toggled(bool)),
             this, SLOT(suodataSuosikit(bool)));
    connect( ui->suodatusEdit, SIGNAL(textChanged(QString)),
             this, SLOT(suodata(QString)));
    connect( ui->view, SIGNAL(clicked(QModelIndex)),
             this, SLOT(klikattu(QModelIndex)));
    connect( ui->view->selectionModel(), SIGNAL(currentChanged(QModelIndex,QModelIndex)),
             this, SLOT(valintaMuuttui(QModelIndex)));

    connect( ui->view, SIGNAL(tiliHiirenAlla(int)), this, SLOT(naytaOhje(int)));


}

TilinValintaDialogi::~TilinValintaDialogi()
{
    delete ui;
}

Tili TilinValintaDialogi::valittu() const
{
    int tiliId = ui->view->currentIndex().data(TiliModel::IdRooli).toInt();
    return kp()->tilit()->tiliIdlla(tiliId);
}

void TilinValintaDialogi::suodata(const QString &alku)
{
    proxyNimi->setFilterFixedString(alku);

    if( tyyppiSuodatin.isEmpty())
    {
        if( alku.isEmpty())
            proxyTyyppi->setFilterFixedString("");
        else
            proxyTyyppi->setFilterRegExp("[ABCD].*");
    }
    else
        proxyTyyppi->setFilterRegExp( tyyppiSuodatin );

    ui->view->paivitaInfo();

    // Jos enää vain yksi tili osuu suodattimeen,
    // valitaan se automaattisesti
    if( ui->view->model()->rowCount(QModelIndex()) == 1)
    {
        ui->view->setCurrentIndex( ui->view->model()->index(0,0) );
    }

}

void TilinValintaDialogi::suodataTyyppi(const QString &regexp)
{
    tyyppiSuodatin = regexp;
    suodata(ui->suodatusEdit->text());
}

void TilinValintaDialogi::suodataSuosikit(bool suodatetaanko)
{
    if( suodatetaanko)
        proxyTila->setFilterFixedString("2");
    else
        proxyTila->setFilterRegExp("[12]");
    ui->view->paivitaInfo();


}

void TilinValintaDialogi::asetaModel(TiliModel *model)
{
    tiliModel = model;
    proxyNimi->setSourceModel( model );
}

void TilinValintaDialogi::klikattu(const QModelIndex &index)
{
    if(index.data(TiliModel::OtsikkotasoRooli) == 0)
    {
        // Valittu tili eikä otsikkoa
        accept();
    }
}

void TilinValintaDialogi::valintaMuuttui(const QModelIndex &index)
{
    ui->valitseNappi->setEnabled( index.isValid() && index.data( TiliModel::OtsikkotasoRooli ).toInt() == 0 );
}

void TilinValintaDialogi::naytaOhje(int tiliId)
{
    Tili tili = tiliModel->tiliIdlla(tiliId);
    QString txt = tili.json()->str("Taydentava");

    if( !tili.json()->str("Kirjausohje").isEmpty() )
    {
        if( !txt.isEmpty())
            txt.append("\n");
        txt.append( tili.json()->str("Kirjausohje"));
    }

    ui->infoMerkki->setVisible( !txt.isEmpty() );
    ui->ohjeLabel->setVisible( !txt.isEmpty() );

    ui->ohjeLabel->setText( txt );
}





Tili TilinValintaDialogi::valitseTili(const QString &alku, const QString &tyyppiSuodatin, TiliModel *model)
{
    TilinValintaDialogi dlg;
    if( model )
        dlg.asetaModel( model );

    dlg.ui->suodatusEdit->setText(alku);
    dlg.suodataTyyppi(tyyppiSuodatin);

    if( dlg.exec())
    {
        return dlg.valittu();
    }
    return Tili();
}

