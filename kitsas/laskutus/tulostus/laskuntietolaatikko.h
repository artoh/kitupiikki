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
#ifndef LASKUNTIETOLAATIKKO_H
#define LASKUNTIETOLAATIKKO_H

#include "model/tosite.h"
#include <QRectF>

class KitsasInterface;
class QPainter;


class LaskunTietoLaatikko
{
public:
    LaskunTietoLaatikko( KitsasInterface* kitsas_);
    void lataa(Tosite &tosite);
    qreal laskeLaatikko(QPainter* painter, qreal leveys);
    void piirra(QPainter* painter);

    qreal korkeus() const { return laatikko_.bottom();}
    qreal leveys() const { return laatikko_.width();}

    void ylatunniste(QPainter *painter);

protected:    
    void piirraLaatikko( QPainter *painter);
    void piirraTekstit( QPainter *painter );
    void piirraHarjoitus( QPainter *painter );
    void piirraTulostusPaiva(QPainter *painter);


    void lisaa(const QString& avain, const QString& arvo);
    void lisaa(const QString& avain, const QDate& pvm);    

    void ylatunnisteNimialue(QPainter* painter);
    void ylatunnisteOtsikko(QPainter* painter);
    void ylatunnistePvmalue(QPainter* painter);


private:
    class TietoRivi {
    public:
        TietoRivi();
        TietoRivi(const QString& otsikko, const QString& tieto);
        QString otsikko() const { return otsikko_;}
        QString tieto() const {return tieto_;}
    private:
        QString otsikko_;
        QString tieto_;
    };

    KitsasInterface* kitsas_;
    QString kieli_;

    int fonttikoko_ = 10;

    qreal rivinKorkeus_;

    int otsikkoleveys_ = 0;    
    QRectF laatikko_;

    QList<TietoRivi> rivit_;
    QString otsikko_;

};


#endif // LASKUNTIETOLAATIKKO_H
