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

#include <QFileDialog>
#include <QFileInfo>

#include <QGraphicsScene>
#include <QGraphicsView>

#include <QSqlQuery>
#include <QByteArray>
#include <QCryptographicHash>

#include <QDebug>

#include "tositewg.h"
#include "db/kirjanpito.h"


TositeWg::TositeWg(TositeModel *tositemodel)
    : QStackedWidget(), model(tositemodel)
{
    QWidget *paasivu = new QWidget();
    ui = new Ui::TositeWg;
    ui->setupUi(paasivu);
    addWidget(paasivu);

    scene = new QGraphicsScene(this);
    view = new QGraphicsView(scene);

    addWidget(view);

    connect(ui->valitseTiedostoNappi, SIGNAL(clicked(bool)), this, SLOT(valitseTiedosto()));
    connect( model->liiteModel(), SIGNAL(modelReset()), this, SLOT(paivita()));
    connect( model->liiteModel(), SIGNAL(rowsInserted(QModelIndex,int,int)), this, SLOT(paivita()));
}

TositeWg::~TositeWg()
{
    delete ui;
}

void TositeWg::valitseTiedosto()
{
    QString polku = QFileDialog::getOpenFileName(this, tr("Valitse tosite"),QString(),tr("Kuvat (*.png *.jpg)"));
    if( !polku.isEmpty())
    {
        QFileInfo info(polku);
        model->liiteModel()->lisaaTiedosto( polku, info.fileName());
    }
}

void TositeWg::paivita()
{
    if( model->liiteModel()->rowCount( QModelIndex()))
    {
        // Näytä tiedosto
        scene->clear();
        QPixmap kuva( model->liiteModel()->data( model->liiteModel()->index(0,0), LiiteModel::Polkurooli ).toString() );
        scene->addPixmap(kuva);
        view->fitInView(0,0,kuva.width(), kuva.height(), Qt::KeepAspectRatio);

        setCurrentIndex(1);
    }
    else
    {
        setCurrentIndex(0);
    }
}


