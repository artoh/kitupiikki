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
#ifndef LASKUMAKSULAATIKKO_H
#define LASKUMAKSULAATIKKO_H

#include <QList>
#include <QSizeF>
#include <QPainter>
#include <QColor>

class QPainter;


class LaskuMaksuLaatikko
{
public:
    LaskuMaksuLaatikko();
    void lisaa(const QString &otsikko, const QString& teksti,
               Qt::AlignmentFlag tasaus = Qt::AlignLeft, bool lihava = false);
    qreal laske(QPainter* painter, qreal leveys);
    void piirra(QPainter* painter, qreal x, qreal y);
    QSizeF koko() const { return koko_;}
    int sarakkeita() const { return sarakkeet_.count();}
    void asetaKehysVari(const QColor& vari);

private:
    class LaatikkoSarake{
    public:
        LaatikkoSarake();
        LaatikkoSarake(QString otsikko, QString teksti,
                      Qt::AlignmentFlag tasaus = Qt::AlignLeft, bool lihava = false);

        QSizeF koko() const {return koko_;}
        void piirra(QPainter* painter, qreal x, qreal y) const;
        QSizeF laske(QPainter* painter);
        Qt::AlignmentFlag tasaus() const { return tasaus_;}

    private:
        QString otsikko_;
        QString teksti_;
        Qt::AlignmentFlag tasaus_;
        bool lihava_;
        QSizeF koko_;
    };

    QList<LaatikkoSarake> sarakkeet_;
    QSizeF koko_;
    qreal vali_ = 0.0;

    QColor variKehys_;

public:
    constexpr static const qreal OTSIKKO_KOKO = 9;
    constexpr static const qreal TEKSTI_KOKO = 10;
};

#endif // LASKUMAKSULAATIKKO_H
