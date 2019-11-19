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
#ifndef LIITETULOSTAJA_H
#define LIITETULOSTAJA_H

#include <QDate>

class QPagedPaintDevice;
class QPainter;

class LiiteTulostaja {
public:
    static bool tulostaLiite(QPagedPaintDevice *printer, QPainter* painter,
                        const QByteArray& data, const QString& tyyppi,
                        const QDate& pvm, const QString& sarja, int tunniste);

    static bool tulostaPdfLiite(QPagedPaintDevice *printer, QPainter* painter,
                        const QByteArray& data,
                        const QDate& pvm, const QString& sarja, int tunniste);

    static void tulostaYlatunniste(QPainter* painter, const QDate& pvm, const QString& sarja, int tunniste);


};

#endif // LIITETULOSTAJA_H
