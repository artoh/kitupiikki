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
#include "liitetulostaja.h"
#include "db/kirjanpito.h"

#include "db/tositetyyppimodel.h"
#include "model/tosite.h"

#include <QPagedPaintDevice>
#include <QPainter>

#include "tools/pdf/pdftoolkit.h"
#include <QGraphicsPixmapItem>
#include <QImage>
#include <QRegularExpression>

#include <QDebug>

int LiiteTulostaja::tulostaLiite(QPagedPaintDevice *printer, QPainter *painter, const QByteArray &data, const QString &tyyppi, const QVariantMap &tosite, bool ensisivu, int sivu, const QString &kieli, int resoluutio, bool aloitaSivunvaihdolla)
{
    if( tyyppi == "application/pdf")
        return tulostaPdfLiite(printer, painter, data, tosite, ensisivu, sivu, kieli, resoluutio, aloitaSivunvaihdolla);
    if( tyyppi.startsWith("image"))
        return tulostaKuvaLiite(printer, painter, data, tosite, ensisivu, sivu, kieli);
    return 0;
}


int LiiteTulostaja::tulostaTiedot(QPagedPaintDevice *printer, QPainter *painter,
                                  const QVariantMap &tosite, int sivu, const QString &kieli,
                                  bool naytaInfo, bool naytaViennit)
{
    QString muistiinpanot = naytaInfo ? tosite.value("info").toString() : QString() ;
    QVariantList viennit = naytaViennit ? tosite.value("viennit").toList() : QVariantList();
    QVariantList loki = tosite.value("loki").toList();

    int sivua = 0;

    painter->setFont(QFont("FreeSans",8));
    int leveys = painter->window().width();
    int sivunKorkeus = painter->window().height();
    int rivinkorkeus = painter->fontMetrics().height();

    if( viennit.isEmpty()) {
        QRect mpRect = painter->boundingRect(0,0,leveys*7/8, sivunKorkeus, Qt::TextWordWrap, muistiinpanot);
        int mpkorkeus = mpRect.height();
        int vientikorkeus = viennit.isEmpty() ? 0 :
                                                (viennit.count() < 8 ? viennit.count() * rivinkorkeus : 6 * rivinkorkeus);
        int korkeus = (rivinkorkeus * 3 + mpkorkeus > 5 * rivinkorkeus ? mpkorkeus : 5 * rivinkorkeus) + vientikorkeus;
        if(sivunKorkeus - painter->transform().dy() < korkeus) {
            printer->newPage();
            painter->resetTransform();
            sivu++; sivua++;
        }
        tulostaYlatunniste(painter, tosite, sivu, kieli);
        painter->translate(0, rivinkorkeus * 3);

        mpRect = painter->boundingRect(0,0,leveys*7/8, sivunKorkeus, Qt::TextWordWrap, muistiinpanot);
        painter->drawText(mpRect, Qt::TextWordWrap, muistiinpanot);

        tulostaKasittelijat(painter, loki);
        painter->translate(0, korkeus + rivinkorkeus * 2);
    } else {
        // Varauduttava pätkimään vientejä. Kuitenkin ensin ylätunniste ja koko sivun levyinen kommentti

        QRect mpRect = painter->boundingRect(0,0,leveys, sivunKorkeus, Qt::TextWordWrap, muistiinpanot);
        qDebug() << muistiinpanot;

        int mpkorkeus = mpRect.height() + 5 * rivinkorkeus;
        int vkorkeus = viennit.count() < 15 ? viennit.count() * rivinkorkeus : rivinkorkeus * 11;
        int korkeus = vkorkeus > mpkorkeus ? vkorkeus : mpkorkeus;

        if(sivunKorkeus - painter->transform().dy() < korkeus) {
            printer->newPage();
            painter->resetTransform();
            sivu++; sivua++;
        }
        tulostaYlatunniste(painter, tosite, sivu, kieli);
        painter->translate(0, rivinkorkeus * 3);

        mpRect = painter->boundingRect(0,0,leveys, sivunKorkeus, Qt::TextWordWrap, muistiinpanot);
        painter->drawText(mpRect, Qt::TextWordWrap, muistiinpanot);
        painter->drawRect(mpRect);

        painter->translate(0, mpRect.height() + rivinkorkeus);
        tulostaKasittelijat(painter, loki);

        // Nyt sitten tarvittaessa pätkitään moneen osaan tätä
        while(!viennit.isEmpty()) {
            int siirto = painter->transform().dy() + rivinkorkeus * 2;
            QVariantList mahtuu;

            while( siirto < sivunKorkeus && !viennit.isEmpty()) {
                mahtuu.append(viennit.takeFirst());
                siirto += rivinkorkeus;
            }            

            tulostaViennit(painter, mahtuu, kieli);
            painter->translate(0, 4 * rivinkorkeus);

            if( !viennit.isEmpty()) {
                printer->newPage();
                painter->resetTransform();
                sivu++; sivua++;
                tulostaYlatunniste(painter, tosite, sivu, kieli);
                painter->translate(0, rivinkorkeus * 3);
                qDebug() << " Uusi sivu " << sivu << " trans " << painter->transform().dy();
            }
        }
    }
    return sivua;

}

int LiiteTulostaja::tulostaPdfLiite(QPagedPaintDevice *printer, QPainter *painter, const QByteArray &data, const QVariantMap& tosite, bool ensisivu, int sivu, const QString& kieli, int resoluutio, bool aloitaSivunvaihdolla)
{
    PdfRendererDocument *document = PdfToolkit::renderer(data);

    if( document->locked()) {
        delete document;
        if(ensisivu)
            return tulostaTiedot(printer, painter, tosite, sivu, kieli, true, true);
        else
            return 0;
    }


    painter->setFont(QFont("FreeSans",8));
    if( aloitaSivunvaihdolla )
        printer->newPage();

    int rivinKorkeus = painter->fontMetrics().height();
    int sivut = 0;

    int pageCount = document->pageCount();
    for(int i=0; i < pageCount; i++)
    {
        painter->resetTransform();
        try {

            QImage image = document->renderPage(i, resoluutio);
            QImage scaled = image.scaled(painter->window().width(), painter->window().height() - 12 * rivinKorkeus, Qt::KeepAspectRatio, Qt::SmoothTransformation);

            painter->drawImage(0, rivinKorkeus * 2, scaled);
            QRect rect(0, rivinKorkeus * 2, painter->window().width(), painter->window().height() - 10 * rivinKorkeus);            

            tulostaYlatunniste(painter, tosite, sivu + (++sivut), kieli);
            painter->translate(0, painter->window().height() - ( ensisivu ? 8 : 1 ) * rivinKorkeus);

        }
            catch (std::bad_alloc&) {            
            delete document;
            return -1;
        }

        if(ensisivu) {
            tulostaAlatunniste(painter, tosite, kieli);
            ensisivu = false;
        }

        if( i < pageCount - 1 )
            printer->newPage();               
    }
    painter->translate(0, painter->window().height() - painter->transform().dy());

    delete document;
    return sivut;
}

int LiiteTulostaja::tulostaKuvaLiite(QPagedPaintDevice *printer, QPainter *painter, const QByteArray &data, const QVariantMap& tosite, bool ensisivu, int sivu, const QString& kieli)
{    
    painter->setFont(QFont("FreeSans",8));
    int rivinKorkeus = painter->fontMetrics().height();

    int sivua = 0;

    try {

        QImage kuva = QImage::fromData(data);
        if(kuva.isNull()) {
            return 0;
        }        

        QImage scaled = kuva.scaled(painter->window().width(), painter->window().height() - 12 * rivinKorkeus, Qt::KeepAspectRatio, Qt::SmoothTransformation);


        if( (painter->transform().dy() + scaled.height() + rivinKorkeus * 12) > painter->window().height() ) {

            printer->newPage();
            painter->resetTransform();
            sivu++; sivua++;
        }


        painter->drawImage(0, rivinKorkeus * 3, scaled);
        tulostaYlatunniste(painter, tosite, sivu + 1, kieli);

        painter->translate(0, rivinKorkeus * 3);
        painter->translate(0, scaled.height() + rivinKorkeus * 4);

        if(ensisivu) {            
            tulostaAlatunniste(painter, tosite, kieli);
            painter->translate(0, 2 * rivinKorkeus);
        }
        painter->translate(0, 2 * rivinKorkeus);
    }
        catch (std::bad_alloc&) {
        return -1;
    }

    return sivua;
}

void LiiteTulostaja::tulostaYlatunniste(QPainter *painter, const QVariantMap &tosite, int sivu, const QString& kieli)
{    

    painter->setFont(QFont("FreeSans",8));
    int leveys = painter->window().width();
    int korkeus = painter->fontMetrics().height() * 2;


    if(!painter->transform().dy()) {
        painter->save();
        int skorkeus = painter->window().height();

        painter->setPen( QPen(Qt::darkGray));
        painter->setFont( QFont("FreeSans",8,QFont::Black));
        painter->translate(0,skorkeus);
        painter->rotate(270);
        painter->translate(0,0-korkeus*3/2);


        QString teksti = sivu > 0 ? QString("%1 %2 %3")
                .arg( kp()->asetukset()->asetus(AsetusModel::OrganisaatioNimi) )
                .arg(QDateTime::currentDateTime().toString("dd.MM.yyyy hh.mm"))
                .arg(tulkkaa("Sivu %1",kieli).arg(sivu)) :
                QString("%1 %2")
                                .arg( kp()->asetukset()->asetus(AsetusModel::OrganisaatioNimi) )
                                .arg(QDateTime::currentDateTime().toString("dd.MM.yyyy"));
        painter->drawText(QRect(0, 0,skorkeus, korkeus*2 ), Qt::AlignHCenter | Qt::AlignTop, teksti );

        if( kp()->asetukset()->onko("Harjoitus") && !kp()->asetukset()->onko("Demo") )
        {
            painter->setPen( QPen(Qt::green));
            painter->setFont( QFont("FreeSans",18,QFont::Black));
            painter->translate(0, 0-korkeus);
            painter->drawText(QRect(0, 0,skorkeus, korkeus ), Qt::AlignHCenter | Qt::AlignVCenter, tulkkaa("HARJOITUS", kieli) );
        }

        painter->restore();
    }

    QString pvm = tosite.value("pvm").toDate().toString("dd.MM.yyyy");
    QString otsikko = tosite.value("otsikko").toString();
    QString kumppani = tosite.value("kumppani").toMap().value("nimi").toString();
    QString tunniste = kp()->tositeTunnus( tosite.value("tunniste").toInt(),
                                           tosite.value("pvm").toDate(),
                                           tosite.value("sarja").toString(),true);

    // Pvm
    painter->setFont(QFont("FreeSans",18, QFont::Bold));
    QRectF pvmRect(0, 0, leveys*3/12, korkeus );
    painter->drawText( pvmRect, pvm, QTextOption(Qt::AlignLeft | Qt::AlignTop));


    QRectF tunnisteRect(leveys * 9 / 12, 0, leveys * 3 / 12, korkeus);
    painter->drawText( tunnisteRect, tunniste, QTextOption(Qt::AlignRight | Qt::AlignTop) );

    painter->setFont(QFont("FreeSans",9, QFont::Bold));    
    QRectF otsikkoRect=painter->boundingRect(leveys * 3 / 12, 0, leveys / 2, korkeus, Qt::AlignHCenter | Qt::AlignTop | Qt::TextWordWrap, otsikko );
    painter->drawText( otsikkoRect, otsikko, QTextOption(Qt::AlignHCenter | Qt::AlignTop) );
    int otsikkokorkeus = otsikkoRect.height();

    painter->setFont(QFont("FreeSans",9));
    QRectF kumppaniRect(leveys * 3/ 12, otsikkokorkeus > korkeus / 2 ? otsikkokorkeus : korkeus / 2 , leveys / 2, korkeus * 3 / 2);
    painter->drawText( kumppaniRect, kumppani, QTextOption(Qt::AlignHCenter | Qt::AlignTop) );



}

void LiiteTulostaja::tulostaAlatunniste(QPainter *painter, const QVariantMap &tosite, const QString& kieli)
{

    painter->setFont(QFont("FreeSans",8));
    int leveys = painter->window().width();
    int korkeus = painter->fontMetrics().height();
    int tositetyyppi = tosite.value("tyyppi").toInt();

    // Kitsaan laskujen infokenttä on tulostettu laskulle, joten sitä ei kannata tulostaa erikseen.
    QString info = tositetyyppi >= TositeTyyppi::MYYNTILASKU && tositetyyppi <= TositeTyyppi::MAKSUMUISTUTUS ?
                "" :
                tosite.value("info").toString();
    QRectF infoRect = painter->boundingRect(leveys/2+korkeus,0,leveys*3/8, painter->window().height(),Qt::TextWordWrap,info);
    painter->drawText(infoRect, Qt::TextWordWrap, info);

    tulostaKasittelijat(painter, tosite.value("loki").toList());

    // Sitten vielä tulostetaan tiliöintiä, jos mahtuvat
    QVariantList viennit = tosite.value("viennit").toList();
    if(viennit.count() > 0 && (viennit.count() + 1) * korkeus < painter->window().height() - painter->transform().dy() ) {
        tulostaViennit(painter, viennit, kieli);
    }

}

void LiiteTulostaja::tulostaViennit(QPainter *painter, const QVariantList &viennit, const QString& kieli)
{
    painter->setFont(QFont("FreeSans",8));
    int korkeus = painter->fontMetrics().height();
    int leveys = painter->window().width() / 2 - korkeus;

    painter->setFont(QFont("FreeSans",6));
    painter->drawText(QRect(leveys/2,0,leveys/4,korkeus), "Debet €", QTextOption(Qt::AlignHCenter | Qt::AlignTop) );
    painter->drawText(QRect(leveys*3/4,0,leveys/4,korkeus), "Kredit €", QTextOption(Qt::AlignHCenter | Qt::AlignTop) );

    int otsakekorkeus = painter->fontMetrics().height();
    int viivay = otsakekorkeus + (korkeus - otsakekorkeus) / 2;
    painter->drawLine(0, viivay, leveys, viivay);
    int tviiva = korkeus * ( 1 + viennit.count());
    painter->drawLine(leveys*3/4, 0, leveys*3/4, tviiva);
    painter->drawLine(leveys/2, 0, leveys/2, tviiva);

    painter->setFont(QFont("FreeSans",8));

    for(const auto& vienti : qAsConst( viennit )) {
        painter->translate(0, painter->fontMetrics().height() );

        QVariantMap map = vienti.toMap();

        Tili* tili = kp()->tilit()->tili(map.value("tili").toInt());
        if(tili) {
            painter->drawText(QRect(0,0,leveys/2,korkeus), tili->nimiNumero(kieli), QTextOption(Qt::AlignLeft | Qt::AlignTop));
        }
        double debet = map.value("debet").toDouble();
        if(qAbs(debet) > 1e-5) {
            painter->drawText(QRect(leveys/2,0,leveys/4-korkeus/2,korkeus), QString("%L1").arg(debet,0,'f',2), QTextOption(Qt::AlignRight | Qt::AlignTop));
        }
        double kredit = map.value("kredit").toDouble();
        if(qAbs(kredit) > 1e-5) {
            painter->drawText(QRect(leveys*3/4,0,leveys/4-korkeus/2,korkeus), QString("%L1").arg(kredit,0,'f',2), QTextOption(Qt::AlignRight | Qt::AlignTop));
        }
    }
}

void LiiteTulostaja::tulostaKasittelijat(QPainter *painter, const QVariantList &loki)
{
    QString nimet;
    QSet<int> idt;

    for(QVariant var : loki) {
        QVariantMap map = var.toMap();
        int tila = map.value("tila").toInt();
        if( tila < Tosite::SAAPUNUT ||
            tila == Tosite::LUONNOS  )
            continue;

        int id = map.value("userid").toInt();
        if( id < 1 || idt.contains(id))
            continue;
        idt.insert(id);

        if(!nimet.isEmpty())
            nimet.append("\n");

        QDateTime aika = map.value("aika").toDateTime();
        nimet.append(aika.toLocalTime().toString("dd.MM.yy "));

        QString nimi = map.value("nimi").toString();
        QStringList etukirjaimet = nimi.split(QRegularExpression("\\W", QRegularExpression::UseUnicodePropertiesOption));
        for(QString& osa : etukirjaimet)
            nimet.append(osa.leftRef(1));

        if( idt.count() > 5)
            break;
    }

    if( idt.isEmpty())
        return;

    painter->setFont(QFont("FreeSans",6));

    int leveys = painter->window().width();
    qreal ad = painter->fontMetrics().height() / 8;
    QRectF nimiRect = painter->boundingRect(0,ad*2,leveys/8-ad*16, painter->window().height(),Qt::TextWordWrap,nimet);

    nimiRect.moveRight(leveys);

    painter->drawText(nimiRect, Qt::TextWordWrap, nimet);
    painter->drawRect(nimiRect.adjusted(-ad,-ad,ad,ad));

}
