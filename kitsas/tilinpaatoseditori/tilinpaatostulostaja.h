/*
   Copyright (C) 2017 Arto Hyvättinen

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

#ifndef TILINPAATOSTULOSTAJA_H
#define TILINPAATOSTULOSTAJA_H

#include <QTextDocument>
#include <QPagedPaintDevice>
#include "db/tilikausi.h"
#include "raportti/raportinkirjoittaja.h"
#include "naytin/esikatseltava.h"

#include "raportti/raporttivalinnat.h"

#include <QVector>
#include <QPagedPaintDevice>
/**
 * @brief Tilinpäätöksen tulostus
*/
class TilinpaatosTulostaja : public QObject, public Esikatseltava
{
    Q_OBJECT
public:
    TilinpaatosTulostaja(Tilikausi tilikausi, const QString& teksti, const QStringList& raportit,
                         const QString& kieli, bool naytaTulostusPvm = true, QObject* parent = nullptr);
    virtual ~TilinpaatosTulostaja() override;

    void nayta();
    void tallenna();


    virtual void tulosta(QPagedPaintDevice *writer) const override;
    virtual QString otsikko() const override;

    static QString kasitteleTaulukot(const QString& teksti);

    static void tulostaKansilehti(QPainter *painter, const QString otsikko, Tilikausi kausi, const QString& kieli);
    static void tulostaKansilehti(QPainter *painter, const QString& otsikko, const QString& alaotsikko, const QString& kieli);

signals:
    void tallennettu();

private:
    void tilaaRaportit();


protected:
    bool tilaaRaportti(const QString& raportinnimi);
    void raporttiSaapuu(const RaportinKirjoittaja& kirjoittaja, const RaporttiValinnat& valinnat);

//protected slots:
//    void raporttiSaapuu(int raportti, RaportinKirjoittaja rk, const QString &otsikko);


protected:
    Tilikausi tilikausi_;
    QVector<RaportinKirjoittaja> kirjoittajat_;
    QVector<RaportinKirjoittaja> lisaKirjoittajat_;
    int tilattuja_;
    QString teksti_;
    QStringList raportit_;
    bool tallenna_;
    QString kieli_;
    bool naytaTulostusPvm_;

};

#endif // TILINPAATOSTULOSTAJA_H
