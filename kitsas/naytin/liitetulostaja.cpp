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

#include <QPagedPaintDevice>
#include <QPainter>

#include <poppler/qt5/poppler-qt5.h>
#include <QGraphicsPixmapItem>
#include <QImage>

bool LiiteTulostaja::tulostaLiite(QPagedPaintDevice *printer, QPainter *painter, const QByteArray &data, const QString &tyyppi, const QVariantMap &tosite, bool ensisivu, const QString &kieli)
{
    painter->save();

    if( tyyppi == "application/pdf")
        return tulostaPdfLiite(printer, painter, data, tosite, ensisivu, kieli);
    if( tyyppi.startsWith("image"))
        return tulostaKuvaLiite(printer, painter, data, tosite, ensisivu, kieli);

    painter->restore();
    return false;
}

bool LiiteTulostaja::tulostaMuistiinpanot(QPagedPaintDevice *printer, QPainter *painter, const QVariantMap &tosite, const QString &kieli)
{
    painter->setFont(QFont("FreeSans",10));
    int rivinKorkeus = painter->fontMetrics().height();
    int leveys = painter->window().width();

    tulostaYlatunniste(painter, tosite, kieli);

    painter->translate(0, rivinKorkeus * 3);
    QString info = tosite.value("info").toString();
    QRectF infoRect = painter->boundingRect(0,0,leveys, painter->window().height(),Qt::TextWordWrap,info);
    painter->drawText(infoRect, Qt::TextWordWrap, info);

    painter->setFont(QFont("FreeSans",8));
    int tilinKorkeus = painter->fontMetrics().height();


    painter->translate(leveys / 4, infoRect.height() + rivinKorkeus * 2);
    QVariantList viennit = tosite.value("viennit").toList();
    if(viennit.count() > 0 && (viennit.count() + 1) * tilinKorkeus < painter->window().height() - painter->transform().dy() )
    {
        tulostaViennit(painter, viennit, kieli);
    }
    painter->translate(-leveys/4, rivinKorkeus * 4);

    return true;
}

bool LiiteTulostaja::tulostaPdfLiite(QPagedPaintDevice *printer, QPainter *painter, const QByteArray &data, const QVariantMap& tosite, bool ensisivu, const QString& kieli)
{
    Poppler::Document *document = Poppler::Document::loadFromData(data);
    document->setRenderHint(Poppler::Document::TextAntialiasing);
    document->setRenderHint(Poppler::Document::Antialiasing);        

    painter->setFont(QFont("FreeSans",8));
    int rivinKorkeus = painter->fontMetrics().height();


#ifndef Q_OS_WINDOWS
    document->setRenderBackend(Poppler::Document::ArthurBackend);
#else
    document->setRenderBackend(Poppler::Document::SplashBackend);
#endif

    int pageCount = document->numPages();
    for(int i=0; i < pageCount; i++)
    {
        painter->resetTransform();
        Poppler::Page *page = document->page(i);

        double vaakaResoluutio =  printer->pageLayout().paintRect(QPageLayout::Point).width() / page->pageSizeF().width() * printer->logicalDpiX();
        double pystyResoluutio = printer->pageLayout().paintRect(QPageLayout::Point).height() / page->pageSizeF().height()  * printer->logicalDpiY();

        double resoluutio = vaakaResoluutio < pystyResoluutio ? vaakaResoluutio : pystyResoluutio;

#ifndef Q_OS_WINDOWS
        page->renderToPainter( painter, resoluutio, resoluutio,
                                           0, 0 - rivinKorkeus * 2 ,page->pageSize().width(), page->pageSize().height());

#else
        QImage image = page->renderToImage(resoluutio, resoluutio);        
        painter->drawImage(0,painter->fontMetrics().height() * 2,image);
#endif

        tulostaYlatunniste(painter, tosite, kieli);
        painter->translate(0, resoluutio / 72 * page->pageSize().height() + 2 * rivinKorkeus);

        if(ensisivu) {
            tulostaAlatunniste(painter, tosite, kieli);
            ensisivu = false;
        }

        if( i < pageCount - 1)
            printer->newPage();       

        delete page;
    }
    delete document;
    return true;
}

bool LiiteTulostaja::tulostaKuvaLiite(QPagedPaintDevice */*printer*/, QPainter *painter, const QByteArray &data, const QVariantMap& tosite, bool ensisivu, const QString& kieli)
{
    painter->resetTransform();
    painter->setFont(QFont("FreeSans",8));
    int rivinKorkeus = painter->fontMetrics().height();
    QRect rect = painter->viewport().adjusted(0,rivinKorkeus * 2,0,rivinKorkeus * 6);

    QImage kuva = QImage::fromData(data);

    QSize size = kuva.size();
    size.scale(rect.size(), Qt::KeepAspectRatio);
    painter->save();
    painter->setViewport( rect.x(), rect.y(),
                         size.width(), size.height());
    painter->setWindow(kuva.rect());
    painter->drawImage(0, 0, kuva);
    painter->restore();
    painter->save();

    tulostaYlatunniste(painter, tosite, kieli);
    if(ensisivu) {
        tulostaAlatunniste(painter, tosite, kieli);
    }


    return !kuva.isNull();
}

void LiiteTulostaja::tulostaYlatunniste(QPainter *painter, const QVariantMap &tosite, const QString& kieli)
{    
    painter->save();
    painter->setFont(QFont("FreeSans",8));
    int leveys = painter->window().width();
    int korkeus = painter->fontMetrics().height() * 2;

    if( kp()->asetukset()->onko("Harjoitus") && !kp()->asetukset()->onko("Demo") )
    {
        painter->save();
        painter->setPen( QPen(Qt::green));
        painter->setFont( QFont("FreeSans",14));
        painter->drawText(QRect(leveys / 16 * 10, 0,leveys/4, korkeus ), Qt::AlignHCenter | Qt::AlignVCenter, tulkkaa("HARJOITUS", kieli) );
        painter->restore();
    }

    QString pvm = tosite.value("pvm").toDate().toString("dd.MM.yyyy");
    QString otsikko = tosite.value("otsikko").toString();
    QString kumppani = tosite.value("kumppani").toMap().value("nimi").toString();
    QString tunniste = kp()->tositeTunnus( tosite.value("tunniste").toInt(),
                                           tosite.value("pvm").toDate(),
                                           tosite.value("sarja").toString());

    // Pvm
    painter->setFont(QFont("FreeSans",18, QFont::Bold));
    QRectF pvmRect(0, 0, leveys*3/12, korkeus );
    painter->drawText( pvmRect, pvm, QTextOption(Qt::AlignLeft | Qt::AlignTop));


    QRectF tunnisteRect(leveys * 9 / 12, 0, leveys * 3 / 12, korkeus);
    painter->drawText( tunnisteRect, tunniste, QTextOption(Qt::AlignRight | Qt::AlignTop) );

    painter->setFont(QFont("FreeSans",9, QFont::Bold));
    QRectF otsikkoRect(leveys * 3 / 12, 0, leveys / 2, korkeus );
    painter->drawText( otsikkoRect, otsikko, QTextOption(Qt::AlignHCenter | Qt::AlignTop) );

    painter->setFont(QFont("FreeSans",9));
    QRectF kumppaniRect(leveys * 3/ 12, korkeus / 2, leveys / 2, korkeus * 3 / 2);
    painter->drawText( kumppaniRect, kumppani, QTextOption(Qt::AlignHCenter | Qt::AlignTop) );
    painter->restore();

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
    QRectF infoRect = painter->boundingRect(leveys/2+korkeus,0,leveys/2-korkeus, painter->window().height(),Qt::TextWordWrap,info);
    painter->drawText(infoRect, Qt::TextWordWrap, info);

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

    for(QVariant vienti : viennit) {
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
