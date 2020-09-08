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
#ifndef REKISTERINVIENTI_H
#define REKISTERINVIENTI_H

#include <QObject>
#include <QQueue>

#include "raportti/raportinkirjoittaja.h"

class QAbstractItemModel;

class RekisterinVienti : public QObject
{
    Q_OBJECT
public:
    static void vieRekisteri(QAbstractItemModel* model, const QString& tiedostonnimi);
protected:
    RekisterinVienti(QAbstractItemModel *model, const QString& tiedostonnimi);
    void haeSeuraava();
    void kumppaniSaapuu(QVariant* data);
    void valmis();

private:
    QString tiedosto_;
    QQueue<int> idt_;
    RaportinKirjoittaja rk_;


signals:

};

#endif // REKISTERINVIENTI_H
