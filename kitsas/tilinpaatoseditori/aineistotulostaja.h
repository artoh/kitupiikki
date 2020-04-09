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

class QProgressDialog;
class QPagedPaintDevice;
class QPainter;

class AineistoTulostaja : public QObject
{
    Q_OBJECT
public:
    AineistoTulostaja(QObject *parent = nullptr);
    ~AineistoTulostaja() override;

    void tallennaAineisto(Tilikausi kausi, const QString& kieli = "fi");

signals:

public slots:

protected:
    void tilaaRaportit();
    void raporttiSaapuu(int raportti, RaportinKirjoittaja rk);
    void tilaaLiitteet();
    void liiteListaSaapuu(QVariant *data);
    void liiteSaapuu(int liiteid, QVariant* var);    
    void tulostaRaportit();
    void tilaaSeuraavaLiite();
    void tulostaLiite(QVariant* data, const QVariantMap& map);

protected:
    Tilikausi tilikausi_;
    QVector<RaportinKirjoittaja> kirjoittajat_;
    QHash<int,QByteArray> liitedatat_;
    QVariantList liitteet_;
    QString kieli_;
    QString polku_;

    int tilattuja_;
    int liitepnt_ = 0;    

    QProgressDialog* progress;
    QPagedPaintDevice *device;
    QPainter *painter = nullptr;

};

#endif // AINEISTOTULOSTAJA_H
