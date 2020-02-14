/*
   Copyright (C) 2018 Arto Hyvättinen

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
#include "erittelyruudukko.h"

#include "db/kirjanpito.h"

#include "myyntilaskuntulostaja.h"

#include <QDebug>
#include <QPagedPaintDevice>

ErittelyRuudukko::ErittelyRuudukko(const QVariantList& rivit , MyyntiLaskunTulostaja *tulostaja)
    : model_(nullptr, rivit),
      tulostaja_(tulostaja)
{
    bool alennuksia  = false;

    for( auto rivi : rivit) {
        QVariantMap map = rivi.toMap();

        if( map.value("aleprosentti").toDouble() > 1e-5)
            alennuksia = true;

        int veroprossa = qRound(map.value("alvprosentti").toDouble() * 100);
        int verokoodi = map.value("alvkoodi").toInt();
        qlonglong brutto = qRound( LaskuRivitModel::riviSumma( map ) * 100);
        int avain = verokoodi * 10000 + veroprossa;

        verokannat_.insert( avain, verokannat_.value(avain, 0) + brutto );
    }

    // Ensin otsikot ja tasaukset
    lisaaSarake("nimike");
    lisaaSarake("lkm",Qt::AlignRight); //Määrä
    lisaaSarake("");    // Yksikkö
    lisaaSarake("anetto", Qt::AlignRight);
    if( alennuksia )
        lisaaSarake("ale%", Qt::AlignRight);
    if( verokannat_.count() > 1)
        lisaaSarake("alv",Qt::AlignRight);
    lisaaSarake("yhteensa", Qt::AlignRight);



    // Sitten itse tiedot
    for(int i=0; i < model_.rowCount(); i++)
    {
        if( qAbs(model_.data( model_.index(i, LaskuRivitModel::BRUTTOSUMMA), Qt::EditRole ).toDouble()) < 1e-5   )
            continue;

        QStringList rivi;

        rivi.append( model_.data( model_.index(i, LaskuRivitModel::NIMIKE), Qt::DisplayRole ).toString() );
        rivi.append( model_.data( model_.index(i, LaskuRivitModel::MAARA), Qt::DisplayRole ).toString() );
        rivi.append( model_.data( model_.index(i, LaskuRivitModel::YKSIKKO), Qt::DisplayRole ).toString() );
        rivi.append( model_.data( model_.index(i, LaskuRivitModel::AHINTA), Qt::DisplayRole ).toString() );

        if( alennuksia )
            rivi.append(model_.data( model_.index(i, LaskuRivitModel::ALE), Qt::DisplayRole ).toString() );
        if( verokannat_.count() > 1)
            rivi.append( model_.data( model_.index(i, LaskuRivitModel::ALV), Qt::DisplayRole ).toString() );

        rivi.append( model_.data( model_.index(i, LaskuRivitModel::BRUTTOSUMMA), Qt::DisplayRole ).toString() );

        ruudut_.append(rivi);
    }

}

void ErittelyRuudukko::tulostaErittely(QPagedPaintDevice *printer, QPainter *painter, qreal marginaali)
{
    // Ensin lasketaan sarakkeiden leveydet

    double mm = printer->width() * 1.00 / printer->widthMM();
    qreal korkeus = painter->window().height() - marginaali;
    qreal sivunleveys = painter->window().width();

    // Aloitetaan otsikoista
    painter->setFont( QFont("FreeSans",8));
    for( QString otsikko : otsikot_)
        leveydet_.append( painter->fontMetrics().width(otsikko) );

    // Sitten data
    painter->setFont( QFont("FreeSans", 10));
    for( const QStringList& rivi : ruudut_)
    {
        for(int i=0; i < rivi.count(); i++)
        {
            qreal leveys = painter->fontMetrics().width( rivi.at(i) );
            if( leveys > leveydet_.at(i))
                leveydet_[i] = leveys;
        }
    }

    // Ensimmäinen sarake on venyvä
    qreal ekanleveyteen = sivunleveys - (leveydet_.count()-1) * VALI * mm;
    for(int i=1; i < leveydet_.count(); i++)
        ekanleveyteen -= leveydet_.at(i);
    leveydet_[0] = ekanleveyteen;
    qreal rk = painter->fontMetrics().height();

    tulostaErittelyOtsikko(printer, painter, false);

    // Tulostetaan erittely

    for(const QStringList& rivi: ruudut_)
    {
        QRectF seliteRect = painter->boundingRect(QRectF(0,0, leveydet_.at(0),korkeus), Qt::TextWordWrap, rivi.at(0) );

        // Tarvittaessa vaihdetaan sivua
        if( painter->transform().dy() + seliteRect.height() > korkeus - 10 * mm)
        {
            painter->drawText(QRectF(0,0,sivunleveys-10*mm,rk), Qt::AlignRight, tulostaja_->t("jatkuu"));
            printer->newPage();
            painter->resetTransform();
            korkeus = painter->window().height();
            tulostaErittelyOtsikko(printer, painter, true);
        }

        painter->drawText( seliteRect, tasaukset_.at(0), rivi.at(0) );

        qreal x = leveydet_.first() + VALI * mm;
        for(int i=1; i < rivi.count(); i++)
        {
            if( i == rivi.count()-1)
                painter->drawText(QRectF(x,0, painter->window().width() - x ,rk), tasaukset_.at(i), rivi.at(i));
            else
                painter->drawText(QRectF(x,0,leveydet_.at(i),rk), tasaukset_.at(i), rivi.at(i));
            x += i==1 ? leveydet_.at(i) + 2 * mm :  leveydet_.at(i) + VALI * mm; // Lyhyempi väli kappalemäärän ja yksikön välissä
        }
        painter->translate( 0, seliteRect.height() > rk ? seliteRect.height() : rk );
    }

    // Alv-erittely

    bool alv = kp()->asetukset()->onko("AlvVelvollinen");
    qreal leveys = painter->window().width();

    // Tarvittaessa koko erittely vaihtaa sivua
    if( painter->transform().dy() + rk * (verokannat_.count() + 2) > korkeus - 10 * mm)
    {
        painter->drawText(QRectF(0,0,sivunleveys-10*mm,rk), Qt::AlignRight, tulostaja_->t("jatkuu"));
        printer->newPage();
        painter->resetTransform();
        korkeus = painter->window().height();
        painter->drawText( QRectF(0,0,sivunleveys/2,painter->fontMetrics().height()), Qt::AlignLeft, kp()->asetukset()->asetus("LaskuAputoiminimi").isEmpty() ? kp()->asetukset()->asetus("Nimi") : kp()->asetukset()->asetus("LaskuAputoiminimi"));
        painter->drawText( QRectF(sivunleveys/2,0,sivunleveys/2, painter->fontMetrics().height()), Qt::AlignRight, kp()->paivamaara().toString("dd.MM.yyyy"));
        painter->translate(0, painter->fontMetrics().height()*2);
    }

    double vero = 0l;

    if( alv )
    {
        painter->translate( 0, rk * 0.5);

        painter->setFont(QFont("FreeSans",8));
        painter->drawText(QRectF( leveys * 5 / 8, 0, leveys / 8, rk), Qt::AlignRight, tulostaja_->t("veroton"));
        painter->drawText(QRectF( leveys * 6 / 8, 0, leveys / 8, rk), Qt::AlignRight, tulostaja_->t("vero"));
        painter->drawText(QRectF( leveys * 7 / 8, 0, leveys / 8, rk), Qt::AlignRight, tulostaja_->t("verollinen"));
        painter->translate(0, painter->fontMetrics().height());
        painter->setFont(QFont("FreeSans",10));

        painter->drawLine(QLineF(2*leveys / 8.0, 0, leveys, 0));

        QMapIterator<int,qlonglong> iter(verokannat_);
        while( iter.hasNext())
        {
            iter.next();
            int alvkoodi = iter.key() / 10000;
            double alvprosentti = ( iter.key() % 10000 ) / 100.0;

            double brutto = iter.value() / 100.0;
            double netto = brutto;


            if( alvkoodi != AlvKoodi::MYYNNIT_NETTO && alvkoodi != AlvKoodi::ALV0)
            {
                painter->drawText(QRectF(2 * leveys / 8,0, leveys/2,rk), Qt::AlignLeft, veroteksti( alvkoodi)  );
            }
            else
            {
                netto = brutto * 100 / ( 100 + alvprosentti );
                painter->drawText(QRectF(2 *leveys / 8,0,leveys/2,rk), Qt::AlignLeft, QString("%1 %2 %").arg(tulostaja_->t("alv")).arg( alvprosentti ) );
                painter->drawText(QRectF(6 *leveys / 8,0,leveys/8,rk), Qt::AlignRight, QString("%L1 €").arg( brutto - netto ,0,'f',2) );

            }

            vero += brutto - netto;

            painter->drawText(QRectF(5 *leveys / 8,0,leveys/8,rk), Qt::AlignRight, QString("%L1 €").arg( netto ,0,'f',2)  );
            painter->drawText(QRectF(7*leveys/8,0,leveys/8,rk), Qt::AlignRight, QString("%L1 €").arg( brutto ,0,'f',2) );
            painter->translate(0, rk);

        }
    }

    // Yhteensä
    painter->translate(0, rk * 0.25);

    qreal yhtviivaAlkaa = alv == true ? 2 * leveys / 8 : 10 * leveys / 16.0; // ilman alviä lyhyempi yhteensä-viiva

    painter->drawLine(QLineF(yhtviivaAlkaa, -0.26 * mm , leveys, -0.26 * mm));
    painter->drawLine(QLineF(yhtviivaAlkaa, 0, leveys, 0));
    painter->drawText(QRectF(yhtviivaAlkaa, 0,leveys/8,rk), Qt::AlignLeft, tulostaja_->t("Yhteensa")  );

    double yhteensa = model_.yhteensa();

    if( alv )
    {
        painter->drawText(QRectF(5 *leveys / 8,0,leveys/8,rk), Qt::AlignRight, QString("%L1 €").arg( yhteensa - vero ,0,'f',2)  );
        painter->drawText(QRectF(6 *leveys / 8,0,leveys/8,rk), Qt::AlignRight, QString("%L1 €").arg(  vero  ,0,'f',2) );
    }
    painter->drawText(QRectF(7*leveys/8,0,leveys/8,rk), Qt::AlignRight, QString("%L1 €").arg(  yhteensa ,0,'f',2) );
    painter->translate(0, 2*rk);


}

QString ErittelyRuudukko::html()
{
    QString txt = "<table width=100% style='margin-top: 2em; margin-bottom: 1em;'>\n<theader><tr>";
    for(QString otsikko : otsikot_)
        txt.append("<th>" + otsikko + "</th>");
    txt.append("</tr></header>\n<tbody>");
    for( QStringList rivi : ruudut_)
    {
        txt.append("<tr>");
        for(int i=0; i < rivi.count(); i++)
        {
            if( tasaukset_.at(i) == Qt::AlignRight)
                txt.append("<td style='text-align: right;'>");
            else
                txt.append("<td>");
            txt.append( rivi.at(i) );
            txt.append("</td>");
        }
        txt.append("</tr>\n");
    }

    txt.append("</tbody></table>\n");
    return txt;
}

void ErittelyRuudukko::lisaaSarake(const QString &otsikontekstinimi, Qt::AlignmentFlag tasaus)
{
    otsikot_.append( otsikontekstinimi.isEmpty() ? QString() :  tulostaja_->t(otsikontekstinimi) );
    tasaukset_.append( tasaus );
}

void ErittelyRuudukko::tulostaErittelyOtsikko(QPagedPaintDevice *printer, QPainter *painter, bool sivuntunniste)
{
    qreal sivunleveys = painter->window().width();
    double mm = printer->width() * 1.00 / printer->widthMM();

    if(sivuntunniste)
    {
        painter->setFont(QFont("FreeSans",10));
        painter->drawText( QRectF(0,0,sivunleveys/2,painter->fontMetrics().height()), Qt::AlignLeft, kp()->asetukset()->asetus("LaskuAputoiminimi").isEmpty() ? kp()->asetukset()->asetus("Nimi") : kp()->asetukset()->asetus("LaskuAputoiminimi"));
        painter->drawText( QRectF(sivunleveys/2,0,sivunleveys/2, painter->fontMetrics().height()), Qt::AlignRight, kp()->paivamaara().toString("dd.MM.yyyy"));
        painter->translate(0, painter->fontMetrics().height()*2);
    }

    painter->setFont(QFont("FreeSans",8));
    qreal x = 0;
    for(int i=0; i<leveydet_.count(); i++)
    {
        qreal leveys = leveydet_.at(i);
        painter->drawText( QRectF(x,0,leveys,painter->fontMetrics().height() ), i==0 ? Qt::AlignLeft : Qt::AlignCenter, otsikot_.at(i) );
        x +=  i==1 ? leveys + 2 * mm :  leveys + VALI * mm;

    }
    painter->translate(0, painter->fontMetrics().height());
    painter->drawLine(QLineF(0,0,sivunleveys,0));
    painter->translate(0, mm);
    painter->setFont(QFont("FreeSans",10));
}

QString ErittelyRuudukko::veroteksti(int verokoodi) const
{
    return tulostaja_->t(QString("alv%1").arg(verokoodi));
}
