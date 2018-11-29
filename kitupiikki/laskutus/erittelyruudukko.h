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

#include "laskumodel.h"
#include "laskuntulostaja.h"

/**
 * @brief Laskussa oleva tuote-erittely
 */
class ErittelyRuudukko
{
public:
    ErittelyRuudukko(LaskuModel *model, LaskunTulostaja *tulostaja);

    void tulostaErittely(QPagedPaintDevice *printer, QPainter *painter, qreal marginaali);
    QString html();

private:
    void lisaaSarake(const QString& otsikontekstinimi, Qt::AlignmentFlag tasaus = Qt::AlignLeft);
    void tulostaErittelyOtsikko(QPagedPaintDevice *printer, QPainter *painter, bool sivuntunniste = false);

    QList<QString> otsikot_;
    QList<QStringList> ruudut_;
    QList<qreal> leveydet_;
    QList<Qt::AlignmentFlag> tasaukset_;

    LaskunTulostaja *tulostaja_;

    enum { VALI = 5};
};

#endif // ERITTELYRUUDUKKO_H
