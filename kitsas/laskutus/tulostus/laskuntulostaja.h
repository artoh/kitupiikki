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

#include <QObject>

class KitsasInterface;
class QPagedPaintDevice;
class QPainter;

class LaskunTulostaja : public QObject
{
    Q_OBJECT
public:
    explicit LaskunTulostaja(KitsasInterface* kitsas, QObject *parent = nullptr);

    void tulosta(const Tosite& tosite,
                 QPagedPaintDevice* printer,
                 QPainter* painter);

    QByteArray pdf(const Tosite& tosite);

signals:

protected:
    void tulostaLuonnos(QPainter* painter, const QString& kieli);

private:
    KitsasInterface *kitsas_;



};

#endif // LASKUNTULOSTAJA_H
