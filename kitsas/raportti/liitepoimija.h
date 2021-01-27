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
#ifndef LIITEPOIMIJA_H
#define LIITEPOIMIJA_H

#include <QObject>
#include <QQueue>
#include <QVariant>

class QPagedPaintDevice;
class QPainter;


class LiitePoimija : public QObject
{
    Q_OBJECT
public:
    LiitePoimija(const QString kieli, QObject *parent = nullptr);

    void poimi(const QDate& alkaa, const QDate& paattyy, int tili=-1, int kohdennus=-1);

protected:
    void viennitSaapuu(QVariant* data);
    void seuraavaTosite();
    void tositeSaapuu(QVariant* data);
    void seuraavaLiite();
    void liiteSaapuu(QVariant* data, const QString& tyyppi);
    void tehty();

signals:
    void valmis();

private:
    QQueue<int> tositeJono_;
    QVariantMap nykyTosite_;
    QQueue<QPair<int,QString>> liiteJono_;

    QString kieli_;
    QString tiedosto_;

    QPagedPaintDevice *device;
    QPainter *painter = nullptr;


};

#endif // LIITEPOIMIJA_H
