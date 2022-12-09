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
#ifndef LASKUNTULOSTAJA_H
#define LASKUNTULOSTAJA_H

#include "model/tosite.h"
#include "tulostusruudukko.h"
#include "laskuntietolaatikko.h"
#include "laskunalaosa.h"
#include <QObject>
#include <QColor>

class KitsasInterface;
class QPagedPaintDevice;
class QPainter;

class LaskunTulostaja : public QObject, public SivunVaihtaja
{
    Q_OBJECT
public:
    explicit LaskunTulostaja(KitsasInterface* kitsas, QObject *parent = nullptr);

    void tulosta(Tosite &tosite,
                 QPagedPaintDevice* printer,
                 QPainter* painter);

    QByteArray pdf( Tosite& tosite);
    void tallennaLaskuLiite( Tosite& tosite);

public:
    virtual qreal vaihdaSivua(QPainter *painter, QPagedPaintDevice *device) override;
    qreal tulostaErittely(const QStringList& erittely, QPainter *painter, QPagedPaintDevice *device, qreal alalaita);    

signals:
    void laskuLiiteTallennettu(QVariantMap tosite);

protected:
    bool tulostaSuomesta(Tosite& tosite);

    qreal tulostaBanner(QPainter* painter, const QString& bannerId);

    void tulostaLuonnos(QPainter* painter);
    qreal tulostaRuudukko(Tosite &tosite, QPainter* painter, QPagedPaintDevice* device, qreal alalaita, bool tulostaKuukaudet = true);
    qreal muistutettavatLaskut(Tosite &tosite, QPainter* painter, QPagedPaintDevice* device, qreal alalaita);    

private:
    KitsasInterface *kitsas_;
    LaskunTietoLaatikko tietoLaatikko_;
    LaskunAlaosa alaOsa_;
    QString kieli_;
    QColor varjoVari_;

};

#endif // LASKUNTULOSTAJA_H
