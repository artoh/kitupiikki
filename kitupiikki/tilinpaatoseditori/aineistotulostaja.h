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
#ifndef AINEISTOTULOSTAJA_H
#define AINEISTOTULOSTAJA_H

#include <QObject>
#include <QHash>

#include "naytin/esikatseltava.h"
#include "db/tilikausi.h"
#include "raportti/raportinkirjoittaja.h"

class AineistoTulostaja : public QObject, public Esikatseltava
{
    Q_OBJECT
public:
    AineistoTulostaja(QObject *parent = nullptr);

    void naytaAineisto(Tilikausi kausi, const QString& kieli = "fi");

    virtual void tulosta(QPagedPaintDevice *writer) const override;
    virtual QString otsikko() const override;

signals:

public slots:

protected:
    void tilaaRaportit();
    void tilaaLiitteet();
    void seuraavaLiite();

protected slots:
    void raporttiSaapuu(int raportti, RaportinKirjoittaja rk);
    void liiteListaSaapuu(QVariant *data);
    void liiteSaapuu(int liiteid, QVariant* var);


protected:
    Tilikausi tilikausi_;
    QVector<RaportinKirjoittaja> kirjoittajat_;
    QHash<int,QByteArray> liitedatat_;
    QVariantList liitteet_;
    QString kieli_;

    int tilattuja_;
    int liitepnt_ = 0;

};

#endif // AINEISTOTULOSTAJA_H
