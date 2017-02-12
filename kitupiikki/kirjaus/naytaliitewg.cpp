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

#include "naytaliitewg.h"
#include "db/kirjanpito.h"


NaytaliiteWg::NaytaliiteWg(QWidget *parent)
    : QStackedWidget(parent)
{
    QWidget *paasivu = new QWidget();
    ui = new Ui::TositeWg;
    ui->setupUi(paasivu);
    addWidget(paasivu);

    scene = new QGraphicsScene(this);
    view = new QGraphicsView(scene);

    addWidget(view);

    connect(ui->valitseTiedostoNappi, SIGNAL(clicked(bool)), this, SLOT(valitseTiedosto()));
}

NaytaliiteWg::~NaytaliiteWg()
{
    delete ui;
}

void NaytaliiteWg::valitseTiedosto()
{
    QString polku = QFileDialog::getOpenFileName(this, tr("Valitse tosite"),QString(),tr("Kuvat (*.png *.jpg)"));
    if( !polku.isEmpty())
    {
        emit lisaaLiite( polku );
    }
}

void NaytaliiteWg::naytaTiedosto(const QString &polku)
{
    if( polku.isEmpty())
    {
        setCurrentIndex(0);
    }
    else
    {
        // Näytä tiedosto
        scene->clear();
        QPixmap kuva( polku );
        scene->addPixmap(kuva);
        view->fitInView(0,0,kuva.width(), kuva.height(), Qt::KeepAspectRatio);

        setCurrentIndex(1);
    }
}


