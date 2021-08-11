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
#include <QVariantMap>

class QPagedPaintDevice;
class QPainter;

class LiiteTulostaja {
public:
    static int tulostaLiite(QPagedPaintDevice *printer, QPainter* painter,
                        const QByteArray& data, const QString& tyyppi,
                        const QVariantMap& tosite, bool ensisivu, int sivu, const QString& kieli = QString(),
                        int resoluutio = 200,
                        bool aloitaSivunvaihdolla = true);

    static bool tulostaMuistiinpanot(QPainter* painter,
                                     const QVariantMap& tosite, int sivu, const QString& kieli = QString());

    static int tulostaTiedot(QPagedPaintDevice *printer, QPainter* painter, const QVariantMap& tosite, int sivu, const QString& kieli = "fi",
                             bool naytaInfo=true, bool naytaViennit=true);

protected:
    static int tulostaPdfLiite(QPagedPaintDevice *printer, QPainter* painter,
                        const QByteArray& data,
                        const QVariantMap& tosite, bool ensisivu, int sivu, const QString& kieli = QString(),
                        int resoluutio = 200, bool aloitaSivunvaihdolla = true);

    static int tulostaKuvaLiite(QPagedPaintDevice *printer, QPainter* painter,
                        const QByteArray& data,
                        const QVariantMap& tosite, bool ensisivu, int sivu, const QString& kieli = QString());



    static void tulostaYlatunniste(QPainter* painter, const QVariantMap& tosite, int sivu, const QString& kieli = QString());
    static void tulostaAlatunniste(QPainter* painter, const QVariantMap& tosite, const QString& kieli = QString());
    static void tulostaViennit(QPainter* painter, const QVariantList& viennit, const QString& kieli = QString());
    static void tulostaKasittelijat(QPainter *painter, const QVariantList& loki);


};

#endif // LIITETULOSTAJA_H
