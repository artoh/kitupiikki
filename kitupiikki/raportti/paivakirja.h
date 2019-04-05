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
#ifndef PAIVAKIRJA_H
#define PAIVAKIRJA_H

#include <QObject>

#include "raportinkirjoittaja.h"

class Paivakirja : public QObject
{
    Q_OBJECT
public:
    explicit Paivakirja(QObject *parent = nullptr);

    void kirjoita(QDate mista, QDate mihin);

signals:
    void valmis(RaportinKirjoittaja rk);

public slots:

private slots:
    void dataSaapuu(QVariantMap *data, int status);

protected:
    RaportinKirjoittaja kirjoittaja;
};

#endif // PAIVAKIRJA_H
