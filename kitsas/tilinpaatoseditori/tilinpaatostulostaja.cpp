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

TilinpaatosTulostaja::TilinpaatosTulostaja(Tilikausi tilikausi, const QString& teksti, const QStringList &raportit, const QString& kieli, QObject *parent)
    : QObject(parent), tilikausi_(tilikausi), teksti_(teksti), raportit_(raportit), kieli_(kieli)
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

    tulostaKansilehti( &painter, "Tilinpäätös", tilikausi_);
    int sivulla = 1;

    // Raportit
    for(auto rk : kirjoittajat_) {
        writer->newPage();
        sivulla += rk.tulosta(writer, &painter, false, sivulla);
    }


    // Liitetiedot, allekirjoitukset yms
    painter.setFont( QFont("FreeSans",10));
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
    tilattuja_ = raportit_.count();
    // Tilaa halutut raportit, jotta ne saadaan käyttöön
    for(QString raportti : raportit_)
        tilaaRaportti(raportti);
}


void TilinpaatosTulostaja::tulostaKansilehti(QPainter *painter, const QString otsikko, Tilikausi kausi)
{

    painter->save();
    painter->setFont(QFont("FreeSans",24,QFont::Bold));

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

    painter->setFont(QFont("FreeSans",24));
    painter->drawText( QRectF(0, sivunkorkeus/3 + rivinkorkeus * 4, sivunleveys, rivinkorkeus ), Qt::TextWordWrap | Qt::AlignCenter | Qt::AlignHCenter, otsikko );
    painter->drawText( QRectF(0, sivunkorkeus/3 + rivinkorkeus * 5, sivunleveys, rivinkorkeus ), Qt::TextWordWrap | Qt::AlignCenter | Qt::AlignHCenter , kausi.kausivaliTekstina());

    painter->setFont(QFont("FreeSans",12));
    QString omaOsoite = kp()->asetus("Katuosoite") + "\n" +
            kp()->asetus("Postinumero") + " " + kp()->asetus("Kaupunki");
    painter->drawText( QRectF(0,sivunkorkeus / 8 * 7, sivunleveys / 3, sivunkorkeus / 8), Qt::TextWordWrap, omaOsoite);
    painter->drawText( QRectF( sivunleveys/3*2, sivunkorkeus / 8 * 7, sivunleveys / 3, rivinkorkeus), Qt::AlignLeft, Kirjanpito::tr("Y-tunnus: %1").arg(kp()->asetukset()->asetus("Ytunnus")));
    painter->drawText( QRectF( sivunleveys/3*2, sivunkorkeus / 8 * 7 + painter->fontMetrics().height(), sivunleveys / 3, rivinkorkeus), Qt::AlignLeft, Kirjanpito::tr("Kotipaikka: %1").arg(kp()->asetukset()->asetus("Kotipaikka")));


    painter->restore();
}

void TilinpaatosTulostaja::tilaaRaportti(const QString &raporttistr)
{
    QRegularExpression raporttiRe("@(?<raportti>.+?)(:(?<optiot>\\w*))?[!](?<otsikko>.+)@");
    QRegularExpressionMatch mats = raporttiRe.match(raporttistr);
    QString raporttitunnus = mats.captured("raportti");
    QString optiot = mats.captured("optiot");
    QString otsikko = mats.captured("otsikko");

    Raportoija::RaportinTyyppi tyyppi = Raportoija::VIRHEELLINEN;

    if( optiot.contains("K"))
        tyyppi = Raportoija::KOHDENNUSLASKELMA;
    else if(optiot.contains("P"))
        tyyppi = Raportoija::PROJEKTILASKELMA;
    bool erittelyt = optiot.contains("E");

    int indeksi = kirjoittajat_.size();
    kirjoittajat_.append(RaportinKirjoittaja());

    Raportoija* raportoija = new Raportoija(raporttitunnus, kieli_ , this, tyyppi);
    Tilikausi edellinen = kp()->tilikausiPaivalle( tilikausi_.alkaa().addDays(-1) );

    if( raportoija->onkoTaseraportti()) {
        raportoija->lisaaTasepaiva( tilikausi_.paattyy() );
        if( edellinen.alkaa().isValid())
            raportoija->lisaaTasepaiva( edellinen.paattyy());
    } else if( optiot.contains("B")) {
        raportoija->lisaaKausi( tilikausi_.alkaa(), tilikausi_.paattyy(),  Raportoija::BUDJETTI );
        raportoija->lisaaKausi( tilikausi_.alkaa(), tilikausi_.paattyy(),  Raportoija::TOTEUTUNUT );
        raportoija->lisaaKausi( tilikausi_.alkaa(), tilikausi_.paattyy(),  Raportoija::BUDJETTIERO );
        raportoija->lisaaKausi( tilikausi_.alkaa(), tilikausi_.paattyy(),  Raportoija::TOTEUMAPROSENTTI );
    } else {
        raportoija->lisaaKausi(tilikausi_.alkaa(), tilikausi_.paattyy());
        if( edellinen.alkaa().isValid())
            raportoija->lisaaKausi(edellinen.alkaa(), edellinen.paattyy());
    }

    connect( raportoija, &Raportoija::valmis, [this,indeksi,otsikko] (RaportinKirjoittaja rk) { this->raporttiSaapuu(indeksi, rk, otsikko); } );
    raportoija->kirjoita(erittelyt,-1);
}


void TilinpaatosTulostaja::raporttiSaapuu(int raportti, RaportinKirjoittaja rk, const QString& otsikko)
{
    rk.asetaOtsikko(otsikko);
    kirjoittajat_[raportti] = rk;
    tilattuja_--;
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


