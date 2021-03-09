/*
   Copyright (C) 2019 Arto Hyvättinen

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
#include "liitepoimija.h"

#include "db/kirjanpito.h"
#include "naytin/liitetulostaja.h"
#include "tilinpaatoseditori/tilinpaatostulostaja.h"

#include <QPdfWriter>
#include <QPainter>
#include <QApplication>
#include <QDesktopServices>
#include <QTimer>

LiitePoimija::LiitePoimija(const QString kieli, int dpi, QObject *parent)
    : QObject(parent), kieli_(kieli), dpi_(dpi)
{
    tiedosto_ = kp()->tilapainen("Tositekooste-%1.pdf").arg(QDateTime::currentDateTime().toString("yyyyMMddhhmmss"));
}

void LiitePoimija::poimi(const QDate &alkaa, const QDate &paattyy, int tili, int kohdennus)
{
    QPdfWriter *writer = new QPdfWriter(tiedosto_);
    writer->setPdfVersion(QPagedPaintDevice::PdfVersion_A1b);
    writer->setTitle(tulkkaa("Tositekooste %1 %2", kieli_).arg(kp()->asetus("Nimi")).arg(QDateTime::currentDateTime().toString("dd.MM.yyyy hh.mm")));
    writer->setCreator(QString("%1 %2").arg(qApp->applicationName()).arg(qApp->applicationVersion()));
    writer->setPageSize( QPdfWriter::A4);

    writer->setPageMargins( QMarginsF(20,10,10,10), QPageLayout::Millimeter );

    painter = new QPainter( writer );
    device = writer;
    painter->setFont(QFont("FreeSans",8));


    KpKysely* kysely = kpk("/viennit");
    kysely->lisaaAttribuutti("alkupvm", alkaa);
    kysely->lisaaAttribuutti("loppupvm", paattyy);
    if(tili > 0)
        kysely->lisaaAttribuutti("tili", tili);
    if(kohdennus > -1)
        kysely->lisaaAttribuutti("kohdennus", kohdennus);

    connect(kysely, &KpKysely::vastaus, this, &LiitePoimija::viennitSaapuu);
    kysely->kysy();
}

void LiitePoimija::viennitSaapuu(QVariant *data)
{
    QVariantList lista = data->toList();
    for(auto &item : lista) {
        QVariantMap map = item.toMap();
        QVariantMap tositeMap = map.value("tosite").toMap();
        int id = tositeMap.value("id").toInt();

        if( !tositeJono_.contains(id))
            tositeJono_.append(id);
    }
    seuraavaTosite();
}

void LiitePoimija::seuraavaTosite()
{
    if( tositeJono_.isEmpty()) {
        tehty();
    } else {
        int id = tositeJono_.dequeue();
        KpKysely* kysely = kpk(QString("/tositteet/%1").arg(id));
        connect(kysely, &KpKysely::vastaus, this, &LiitePoimija::tositeSaapuu);
        kysely->kysy();
    }
}

void LiitePoimija::tositeSaapuu(QVariant *data)
{
    nykyTosite_ = data->toMap();
    QVariantList liitelista = nykyTosite_.value("liitteet").toList();
    for(auto &liite : liitelista) {
        QVariantMap liiteMap = liite.toMap();
        QString tyyppi = liiteMap.value("tyyppi").toString();
        if( tyyppi == "application/pdf" || tyyppi == "image/jpeg") {
            liiteJono_.enqueue(qMakePair(liiteMap.value("id").toInt(), tyyppi));
        }
    }
    if( liiteJono_.isEmpty()) {
        seuraavaTosite();
    } else {
        seuraavaLiite();
    }
}


void LiitePoimija::seuraavaLiite()
{
    if( liiteJono_.isEmpty()) {
        seuraavaTosite();
    } else {
        QPair<int,QString> liitetieto = liiteJono_.dequeue();
        KpKysely *liiteHaku = kpk(QString("/liitteet/%1").arg(liitetieto.first));
        connect( liiteHaku, &KpKysely::vastaus,
                 [this, liitetieto] (QVariant* data) { this->liiteSaapuu(data, liitetieto.second); });
        liiteHaku->kysy();
    }
}

void LiitePoimija::liiteSaapuu(QVariant *data, const QString &tyyppi)
{
    QByteArray ba = data->toByteArray();
    if( ekatulostettu_ )
        device->newPage();

    LiiteTulostaja::tulostaLiite(
                device, painter,
                ba,
                tyyppi,
                nykyTosite_,
                false,
                -1000,
                kieli_,
                dpi_);

    ekatulostettu_ = true;
    tulostettu_++;

    seuraavaLiite();
}

void LiitePoimija::tehty()
{
    painter->end();
    delete painter;

    painter = nullptr;
    QString tnimi = tiedosto_;

    if(tulostettu_) {
        QTimer::singleShot(250, [tnimi] {QDesktopServices::openUrl(QUrl::fromLocalFile(tnimi));});
        emit valmis();
        kp()->odotusKursori(false);
    } else {
        emit tyhja();
    }
}
