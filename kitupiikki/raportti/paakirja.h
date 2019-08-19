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
#ifndef PAAKIRJA_H
#define PAAKIRJA_H

#include <QObject>

#include "raportinkirjoittaja.h"
#include "db/tili.h"
#include "db/tilikausi.h"

class Paakirja : public QObject
{
    Q_OBJECT
public:
    explicit Paakirja(QObject *parent = nullptr);

    void kirjoita(const QDate& mista, const QDate& mihin, int optiot = 0,
                  int kohdennuksella = -1,
                  int tililta = 0);

    enum { TulostaKohdennukset  = 0b00100 ,
           TulostaSummat        = 0b01000 ,
           SamaTilikausi        = 0b10000
         };

signals:
    void valmis(RaportinKirjoittaja rk);

public slots:

private slots:
    void saldotSaapuu(QVariant *data);
    void viennitSaapuu(QVariant *data);

protected:
    void kirjoitaDatasta();
    void aloitaTili(int tilinumero = 0);
    void kirjoitaVienti(QVariantMap map);

protected:
    RaportinKirjoittaja rk;

    QVariantMap saldot_;
    QVariantList viennit_;

    int saapuneet_ = 0;

    Tili nykytili_ ;
    qlonglong debetSumma_ = 0l;
    qlonglong kreditSumma_ = 0l;
    qlonglong kaikkiDebet_ = 0l;
    qlonglong kaikkiKredit_ = 0l;
    qlonglong saldo_ = 0l;

    int optiot_;
    Tilikausi oletustilikausi_;
};

#endif // PAAKIRJA_H
