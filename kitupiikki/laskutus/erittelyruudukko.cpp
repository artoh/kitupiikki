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


ErittelyRuudukko::ErittelyRuudukko(LaskuModel *model, LaskunTulostaja *tulostaja)
    : tulostaja_(tulostaja)
{
    bool kaikki = !kp()->asetukset()->onko("LaskuLyhyetRivit");
    bool alennuksia  = model->onkoAlennuksia();

    // Ensin otsikot ja tasaukset
    lisaaSarake("nimike");
    lisaaSarake("lkm",Qt::AlignRight); //Määrä
    lisaaSarake("");    // Yksikkö
    lisaaSarake("anetto", Qt::AlignRight);
    if( alennuksia )
        lisaaSarake("ale%", Qt::AlignRight);
    if( alennuksia && kaikki)
        lisaaSarake("alennus", Qt::AlignRight);
    if( kaikki )
        lisaaSarake("netto", Qt::AlignRight);
    if( model->alverittely().count() > 1)
        lisaaSarake("alv",Qt::AlignRight);
    if( kaikki )
        lisaaSarake("vero", Qt::AlignRight);
    lisaaSarake("yhteensa", Qt::AlignRight);

    // Sitten itse tiedot
    for(int i=0; i < model->rowCount(QModelIndex());i++)
    {
        int yhtsnt = model->data( model->index(i, LaskuModel::BRUTTOSUMMA), Qt::EditRole ).toInt();
        if( !yhtsnt)
            continue;   // Ohitetaan tyhjät rivit

        QStringList rivi;
        double nettosnt = model->data( model->index(i, LaskuModel::NIMIKE), LaskuModel::NettoRooli ).toDouble();

        rivi.append( model->data( model->index(i, LaskuModel::NIMIKE), Qt::DisplayRole ).toString() );
        rivi.append( model->data( model->index(i, LaskuModel::MAARA), Qt::DisplayRole ).toString() );
        rivi.append( model->data( model->index(i, LaskuModel::YKSIKKO), Qt::DisplayRole ).toString() );
        rivi.append( model->data( model->index(i, LaskuModel::AHINTA), Qt::DisplayRole ).toString() );

        if( alennuksia )
            rivi.append(model->data( model->index(i, LaskuModel::ALE), Qt::DisplayRole ).toString() );
        if( alennuksia && kaikki )  // Euromääräinen alennus
        {
            qlonglong alennus = model->data( model->index(i, 0), LaskuModel::AlennusRooli).toLongLong();
            rivi.append( alennus ? QString("%L1 €").arg( (alennus / 100.0) ,0,'f',2) : QString()  );
        }
        if( kaikki )            
             rivi.append(  nettosnt > 0 ? QString("%L1 €").arg( ( nettosnt / 100.0) ,0,'f',2) : QString());
        if( model->alverittely().count() > 1)
            rivi.append( model->data( model->index(i, LaskuModel::ALV), Qt::DisplayRole ).toString() );
        if( kaikki )
        {
            qlonglong vero = model->data( model->index(i, LaskuModel::NIMIKE), LaskuModel::VeroRooli ).toLongLong();
            rivi.append( vero > 0 ? QString("%L1 €").arg( (  vero / 100.0) ,0,'f',2) : QString() );
        }
        rivi.append( model->data( model->index(i, LaskuModel::BRUTTOSUMMA), Qt::DisplayRole ).toString() );

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
    painter->setFont( QFont("Sans",8));
    for( QString otsikko : otsikot_)
        leveydet_.append( painter->fontMetrics().width(otsikko) );

    // Sitten data
    painter->setFont( QFont("Sans", 10));
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
            painter->drawText(QRectF(x,0,leveydet_.at(i),rk), tasaukset_.at(i), rivi.at(i));
            x += i==1 ? leveydet_.at(i) + 2 * mm :  leveydet_.at(i) + VALI * mm; // Lyhyempi väli kappalemäärän ja yksikön välissä
        }
        painter->translate( 0, seliteRect.height() > rk ? seliteRect.height() : rk );
    }

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
        painter->setFont(QFont("Sans",10));
        painter->drawText( QRectF(0,0,sivunleveys/2,painter->fontMetrics().height()), Qt::AlignLeft, kp()->asetukset()->asetus("LaskuAputoiminimi").isEmpty() ? kp()->asetukset()->asetus("Nimi") : kp()->asetukset()->asetus("LaskuAputoiminimi"));
        painter->drawText( QRectF(sivunleveys/2,0,sivunleveys/2, painter->fontMetrics().height()), Qt::AlignRight, kp()->paivamaara().toString("dd.MM.yyyy"));
        painter->translate(0, painter->fontMetrics().height()*2);
    }

    painter->setFont(QFont("Sans",8));
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
    painter->setFont(QFont("Sans",10));
}
