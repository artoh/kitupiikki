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


TositeWg::TositeWg(Kirjanpito *kp) : QStackedWidget(), kirjanpito(kp)
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

TositeWg::~TositeWg()
{
    delete ui;
}

QString TositeWg::tositeTunniste() const
{
    return QString();
}

bool TositeWg::onkoTiedostoa() const
{
    return (!alkuperainentiedostopolku.isEmpty()) && (!uusitiedostopolku.isEmpty());
}



bool TositeWg::tallennaTosite(int tositeId)
{
    if( uusitiedostopolku.isEmpty())
        return false;     // Tositetiedosto ei vaihtunut


    // Uuden tallentaminen
    QFileInfo info( uusitiedostopolku );
    QString tnimi = QString("%1.%2")
            .arg(tositeId,8,10,QChar('0'))
            .arg( info.suffix());

    QString kopiopolku = kirjanpito->hakemisto().absoluteFilePath("tositteet/" + tnimi);

    QFile tiedosto(uusitiedostopolku);
    tiedosto.open(QIODevice::ReadOnly);
    QByteArray sisalto = tiedosto.readAll();
    tiedosto.close();

    // Lasketaan SHA256-tiiviste eheyden varmistamiseksi
    QCryptographicHash hash(QCryptographicHash::Sha256);
    hash.addData(sisalto);

    QFile uusitiedosto(kopiopolku);
    uusitiedosto.open(QIODevice::WriteOnly);
    uusitiedosto.write(sisalto);
    uusitiedosto.close();


    qDebug() << kopiopolku;

    QSqlQuery kysely;

    kysely.prepare("UPDATE tosite SET tiedosto=:tiedosto, sha=:sha WHERE id=:id");
    kysely.bindValue(":tiedosto", tnimi);
    kysely.bindValue(":id", tositeId);
    kysely.bindValue(":sha", hash.result().toHex() );
    kysely.exec();

    return true;

}

void TositeWg::lataaTosite(const QString &tositetiedostonpolku)
{
    uusitiedostopolku = tositetiedostonpolku;
    naytaTosite(uusitiedostopolku);
}

void TositeWg::valitseTiedosto()
{
    QString polku = QFileDialog::getOpenFileName(this, tr("Valitse tosite"),QString(),tr("Kuvat (*.png *.jpg)"));
    if( !polku.isEmpty())
        lataaTosite(polku);
}

void TositeWg::tyhjenna(const QString &tositenumero, const QString &tositetiedosto)
{
    qDebug() << " Tosite " << tositenumero << " : " << tositetiedosto;

    uusitiedostopolku.clear();

    if( tositetiedosto.isEmpty())
    {
        setCurrentIndex(0);
        alkuperainentiedostopolku = QString();
    }
    else
    {
        alkuperainentiedostopolku = kirjanpito->hakemisto().absoluteFilePath("tositteet/" + tositetiedosto);
        naytaTosite( alkuperainentiedostopolku );
    }
}

void TositeWg::naytaTosite(const QString &tositetiedostonpolku)
{
    scene->clear();
    QPixmap kuva(tositetiedostonpolku);
    scene->addPixmap(kuva);
    view->fitInView(0,0,kuva.width(), kuva.height(), Qt::KeepAspectRatio);

    setCurrentIndex(1);
}


