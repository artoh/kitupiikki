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
#ifndef TILIKARTTALISTAAJA_H
#define TILIKARTTALISTAAJA_H

#include "db/tilikausi.h"
#include "raportinkirjoittaja.h"
#include <QObject>



class TiliKarttaListaaja : public QObject
{
    Q_OBJECT
public:
    enum KarttaValinta
    {
        KAIKKI_TILIT,
        KAYTOSSA_TILIT,
        KIRJATUT_TILIT,
        SUOSIKKI_TILIT
    };

    explicit TiliKarttaListaaja(QObject *parent = nullptr);
    void kirjoita(KarttaValinta valinta, const Tilikausi& tilikaudelta, bool otsikot,
                  bool tulostatyypit, const QDate& saldopvm, bool kirjausohjeet);


signals:
    void valmis(RaportinKirjoittaja rk);

protected slots:
    void saldotSaapuu(QVariant *data);

protected:
    KarttaValinta valinta_;
    Tilikausi tilikausi_;
    bool otsikot_;
    bool tyypit_;
    QDate saldopvm_;
    bool kirjausohjeet_;
};

#endif // TILIKARTTALISTAAJA_H
