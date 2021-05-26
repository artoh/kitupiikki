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
#ifndef TASEERITTELIJA_H
#define TASEERITTELIJA_H

#include "raportinkirjoittaja.h"

#include "raportteri.h"

class TaseErittelija : public Raportteri
{
    Q_OBJECT
public:
    explicit TaseErittelija(QObject *parent = nullptr, const QString& kielikoodi = QString());

    void kirjoita(const QDate &mista, const QDate& mihin);

public slots:

protected slots:
    void dataSaapuu(QVariant *data);

protected:
    void lisaaTositeTunnus(RaporttiRivi* rivi, const QVariantMap& map);

    QDate mista_;
    QDate mihin_;
};

#endif // TASEERITTELIJA_H
