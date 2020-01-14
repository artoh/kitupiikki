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
#ifndef ERITTELYRUUDUKKO_H
#define ERITTELYRUUDUKKO_H

#include <QList>
#include <QPainter>

#include "laskurivitmodel.h"

class MyyntiLaskunTulostaja;
class QPagedPaintDevice;
class QPainter;

/**
 * @brief Laskussa oleva tuote-erittely
 */
class ErittelyRuudukko
{
public:
    ErittelyRuudukko(const QVariantList& rivit , MyyntiLaskunTulostaja *tulostaja);

    void tulostaErittely(QPagedPaintDevice *printer, QPainter *painter, qreal marginaali);
    QString html();

private:
    void lisaaSarake(const QString& otsikontekstinimi, Qt::AlignmentFlag tasaus = Qt::AlignLeft);
    void tulostaErittelyOtsikko(QPagedPaintDevice *printer, QPainter *painter, bool sivuntunniste = false);

    QString veroteksti(int verokoodi) const;

    QList<QString> otsikot_;
    QList<QStringList> ruudut_;
    QList<qreal> leveydet_;
    QList<Qt::AlignmentFlag> tasaukset_;
    QMap<int,qlonglong> verokannat_;

    LaskuRivitModel model_;
    MyyntiLaskunTulostaja *tulostaja_;


    enum { VALI = 5};
};

#endif // ERITTELYRUUDUKKO_H
