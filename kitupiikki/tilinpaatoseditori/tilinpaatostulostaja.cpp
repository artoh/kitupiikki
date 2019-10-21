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

#include <QDesktopServices>
#include <QFile>
#include <QPainter>

#include <QPdfWriter>

#include <QTextDocument>
#include <QAbstractTextDocumentLayout>

#include <QRegularExpression>
#include <QRegularExpressionMatch>
#include <QRegularExpressionMatchIterator>

#include <QDebug>
#include <QApplication>

#include "tilinpaatostulostaja.h"
#include "db/kirjanpito.h"

#include "raportti/raportoija.h"

#include <cmath>

TilinpaatosTulostaja::TilinpaatosTulostaja(Tilikausi tilikausi, const QString& teksti, const QStringList &raportit, QObject *parent)
    : QObject(parent), tilikausi_(tilikausi), teksti_(teksti), raportit_(raportit)
{

}

TilinpaatosTulostaja::~TilinpaatosTulostaja()
{

}

void TilinpaatosTulostaja::nayta()
{
    tallenna_ = false;
    tilaaRaportit();
}

void TilinpaatosTulostaja::tallenna()
{
    tallenna_ = true;
    tilaaRaportit();
}

void TilinpaatosTulostaja::tulosta(QPagedPaintDevice *writer) const
{
    writer->setPageSize( QPdfWriter::A4);

    writer->setPageMargins( QMarginsF(25,10,10,10), QPageLayout::Millimeter );
    QPainter painter( writer );

    tulostaKansilehti( &painter);
    int sivulla = 1;

    // Raportit
    for(auto rk : kirjoittajat_) {
        writer->newPage();
        rk.asetaOtsikko(tr("%1 (TILINPÄÄTÖS)").arg(rk.otsikko().toUpper()));
        sivulla += rk.tulosta(writer, &painter, false, sivulla);
    }


    // Liitetiedot, allekirjoitukset yms
    painter.setFont( QFont("Sans",10));
    int rivinkorkeus = painter.fontMetrics().height();
    RaportinKirjoittaja kirjoittaja;
    kirjoittaja.asetaOtsikko("TILINPÄÄTÖS");
    kirjoittaja.asetaKausiteksti( tilikausi_.kausivaliTekstina());

    QTextDocument doc;
    // Sivutetaan niin, että ylätunniste mahtuu

    QSizeF sivunkoko( painter.viewport().width()  ,  painter.viewport().height() - rivinkorkeus * 4 );

    doc.documentLayout()->setPaintDevice( painter.device() );
    doc.setPageSize( sivunkoko );
    doc.setHtml( teksti_ );

    int pages = qRound(std::ceil( doc.size().height() / sivunkoko.height()  ));
    for( int i=0; i < pages; i++)
    {
        writer->newPage();
        painter.save();
        kirjoittaja.tulostaYlatunniste( &painter, sivulla);
        painter.drawLine(0,0,qRound(sivunkoko.width()),0);
        painter.translate(0, rivinkorkeus );

        painter.translate(0, 0 - i * sivunkoko.height() );

        doc.drawContents( &painter, QRectF(0, i * sivunkoko.height(),
                                           doc.textWidth(), sivunkoko.height() ) );
        painter.restore();
        sivulla++;
    }
    painter.end();

}

QString TilinpaatosTulostaja::otsikko() const
{
    return tr("Tilinpäätös %1").arg(tilikausi_.kausivaliTekstina());
}

void TilinpaatosTulostaja::tilaaRaportit()
{
    // Tilaa halutut raportit, jotta ne saadaan käyttöön
    tilattuja_ = raportit_.count();
    for(QString raportti : raportit_)
        tilaaRaportti(raportti);
}


void TilinpaatosTulostaja::tulostaKansilehti(QPainter *painter) const
{

    painter->save();
    painter->setFont(QFont("Sans",24,QFont::Bold));

    int sivunleveys = painter->window().width();
    int rivinkorkeus = painter->fontMetrics().height();
    int sivunkorkeus = painter->window().height();

    if( !kp()->logo().isNull()  )
    {
        double skaala = (1.00 *  kp()->logo().width() ) / kp()->logo().height();
        double leveys = rivinkorkeus * 4 * skaala;
        double korkeus = rivinkorkeus * 4;

        if( leveys > sivunleveys * 10 / 11)
        {
            leveys = sivunleveys * 10 / 11;
            korkeus = leveys / skaala;
        }

        painter->drawImage( QRectF((sivunleveys - leveys) / 2, sivunkorkeus / 3 - rivinkorkeus * 4, leveys , korkeus),
                              kp()->logo() );
    }
    painter->drawText( QRectF(0, sivunkorkeus/3, sivunleveys, rivinkorkeus * 2), Qt::TextWordWrap | Qt::AlignCenter | Qt::AlignHCenter, kp()->asetukset()->asetus("Nimi"));

    painter->setFont(QFont("Sans",24));
    painter->drawText( QRectF(0, sivunkorkeus/3 + rivinkorkeus * 4, sivunleveys, rivinkorkeus ), Qt::TextWordWrap | Qt::AlignCenter | Qt::AlignHCenter, kp()->tr("Tilinpäätös") );
    painter->drawText( QRectF(0, sivunkorkeus/3 + rivinkorkeus * 5, sivunleveys, rivinkorkeus ), Qt::TextWordWrap | Qt::AlignCenter | Qt::AlignHCenter , tilikausi_.kausivaliTekstina());

    painter->setFont(QFont("Sans",12));
    painter->drawText( QRectF(0,sivunkorkeus / 8 * 7, sivunleveys / 3, sivunkorkeus / 8), Qt::TextWordWrap, kp()->asetukset()->asetus("Osoite"));
    painter->drawText( QRectF( sivunleveys/3*2, sivunkorkeus / 8 * 7, sivunleveys / 3, rivinkorkeus), Qt::AlignLeft, kp()->tr("Y-tunnus: %1").arg(kp()->asetukset()->asetus("Ytunnus")));
    painter->drawText( QRectF( sivunleveys/3*2, sivunkorkeus / 8 * 7 + painter->fontMetrics().height(), sivunleveys / 3, rivinkorkeus), Qt::AlignLeft, kp()->tr("Kotipaikka: %1").arg(kp()->asetukset()->asetus("Kotipaikka")));


    painter->restore();
}

void TilinpaatosTulostaja::tilaaRaportti(const QString &raportinnimi)
{
    int indeksi = kirjoittajat_.size();
    kirjoittajat_.append(RaportinKirjoittaja());

    Raportoija* raportoija = new Raportoija(raportinnimi, "fi", this);
    if( raportoija->onkoTaseraportti())
        raportoija->lisaaTasepaiva( tilikausi_.paattyy() );
    else
        raportoija->lisaaKausi(tilikausi_.alkaa(), tilikausi_.paattyy());

    connect( raportoija, &Raportoija::valmis, [this,indeksi] (RaportinKirjoittaja rk) { this->raporttiSaapuu(indeksi, rk); } );
    raportoija->kirjoita(false,-1);

    qDebug() << "Tilattu raportti " + raportinnimi;
}

void TilinpaatosTulostaja::raporttiSaapuu(int raportti, RaportinKirjoittaja rk)
{
    kirjoittajat_[raportti] = rk;
    tilattuja_--;
    qDebug() << "Raportti saapui " << raportti << " Odotetaan " << tilattuja_;
    if( !tilattuja_) {
        if( tallenna_) {
            QMap<QString,QString> meta;
            meta.insert("Filename",tr("Tilinpäätös %1.pdf").arg(tilikausi_.pitkakausitunnus()));
            meta.insert("Content-type","application/pdf");

            KpKysely* kysely = kpk(QString("/liitteet/0/TP_%1").arg(tilikausi_.paattyy().toString(Qt::ISODate)), KpKysely::PUT);
            connect( kysely, &KpKysely::vastaus, this, &TilinpaatosTulostaja::tallennettu);
            kysely->lahetaTiedosto(pdf(), meta);

        } else
            esikatsele();
    }

}

void TilinpaatosTulostaja::kirjoita()
{

}
