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
#include <QMessageBox>

LiitePoimija::LiitePoimija(const QString kieli, int dpi, QObject *parent)
    : QObject(parent), kieli_(kieli), dpi_(dpi),
      pdfDoc_{new QPdfDocument(this)},
      puskuri_{new QBuffer(this)}
{
    tiedosto_ = kp()->tilapainen("Tositekooste-%1.pdf").arg(QDateTime::currentDateTime().toString("yyyyMMddhhmmss"));
    connect( pdfDoc_, &QPdfDocument::statusChanged, this, &LiitePoimija::pdfTilaMuuttuu );
}

void LiitePoimija::poimi(const QDate &alkaa, const QDate &paattyy, int tili, int kohdennus, PoimintaRajaus rajaus)
{
    rajaus_ = rajaus;

    QPdfWriter *writer = new QPdfWriter(tiedosto_);
    writer->setPdfVersion(QPagedPaintDevice::PdfVersion_A1b);
    writer->setTitle(tulkkaa("Tositekooste %1 %2", kieli_).arg(kp()->asetukset()->nimi(), QDateTime::currentDateTime().toString("dd.MM.yyyy hh.mm")));
    writer->setCreator(QString("%1 %2").arg(qApp->applicationName(), qApp->applicationVersion()));
    writer->setPageSize( QPageSize(QPageSize::A4));
    writer->setResolution(dpi_);

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

        if( rajaus_ != Kaikki) {
            if( map.value("tili").toString() < "2")  continue;  // Ei tasetileille
            if( rajaus_ == Tulot) {
                if( Euro(map.value("debet").toString())) continue; // Ei menoja
            } else if( rajaus_ == Menot) {
                if( Euro(map.value("kredit").toString())) continue; // Ei tuloja
            }
        }



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
        connect( liiteHaku, &KpKysely::vastaus, this,
                 [this, liitetieto] (QVariant* data) { this->liiteSaapuu(data, liitetieto.second); });
        liiteHaku->kysy();
    }
}

void LiitePoimija::liiteSaapuu(QVariant *data, const QString &tyyppi)
{

    if( tyyppi == "application/pdf") {
        puskuri_->close();
        bytes_ = data->toByteArray();
        puskuri_->setBuffer(&bytes_);
        puskuri_->open(QIODevice::ReadOnly);
        pdfDoc_->load(puskuri_);
    } else if( tyyppi.startsWith("image")) {
        QByteArray ba = data->toByteArray();
        LiiteTulostaja::tulostaKuvaLiite(
                    device, painter, ba, nykyTosite_, false, -1000, kieli_ );
        ekatulostettu_ = true;
        tulostettu_++;
        seuraavaLiite();
    } else {
        seuraavaLiite();
    }
}

void LiitePoimija::pdfTilaMuuttuu(QPdfDocument::Status tila)
{
    if( tila == QPdfDocument::Status::Ready) {
        painter->setFont(QFont("FreeSans",8));

        if( ekatulostettu_)
            device->newPage();

        int rivinKorkeus = painter->fontMetrics().height();

        int pageCount = pdfDoc_->pageCount();
        for(int i=0; i < pageCount; i++)
        {
            painter->resetTransform();
            QSizeF koko = pdfDoc_->pagePointSize(i);
            QSizeF kohde( painter->window().width(), painter->window().height());
            koko.scale(kohde, Qt::KeepAspectRatio);

            QPdfDocumentRenderOptions options;
            if( koko.width() > koko.height())
                options.setRotation(QPdfDocumentRenderOptions::Rotation::Clockwise270);

            QImage image = pdfDoc_->render(i, kohde.toSize(), options);
            painter->drawImage(0, rivinKorkeus*2, image);

            LiiteTulostaja::tulostaYlatunniste(painter, nykyTosite_, -1000 , kieli_);
            painter->translate(0, painter->window().height() - ( i ? 1 : 8 ) * rivinKorkeus);

            if( i < pageCount - 1 )
                device->newPage();
        }
        painter->translate(0, painter->window().height() - painter->transform().dy());

        ekatulostettu_ = true;
        tulostettu_++;

        seuraavaLiite();
    } else if( tila == QPdfDocument::Status::Error) {
        seuraavaLiite();
    }
}

void LiitePoimija::tehty()
{
    painter->end();
    delete painter;

    painter = nullptr;

    if(tulostettu_) {        
        QTimer::singleShot(250, this, &LiitePoimija::avaa);
    } else {
        emit tyhja();
        kp()->odotusKursori(false);
        QMessageBox::information(nullptr, tr("Tositekooste"), tr("Tekemilläsi valinnoilla muodostuu tyhjä tositekooste"));
    }
}

void LiitePoimija::avaa()
{
    if( !QDesktopServices::openUrl( QUrl(tiedosto_) ) ) {
            QMessageBox::critical(nullptr, tr("Tiedoston näyttäminen epäonnistui"),
                                  tr("Kitsas ei saanut käynnistettyä ulkoista ohjelmaa tiedoston %1 näyttämiseksi.").arg(tiedosto_ ));
        }
    emit valmis();
    kp()->odotusKursori(false);
}
