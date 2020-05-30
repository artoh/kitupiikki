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
#include "palkkafituonti.h"
#include "db/kirjanpito.h"
#include "model/tositevienti.h"
#include "db/tositetyyppimodel.h"
#include <QJsonDocument>
#include <QDebug>

PalkkaFiTuonti::PalkkaFiTuonti()
{
    QString a = kp()->asetus("PalkkaFiTilit");
    QByteArray ba = a.toUtf8();
    QJsonParseError error;
    QJsonDocument doc = QJsonDocument::fromJson(ba, &error);
    qDebug() << error.errorString();
    QVariant var = doc.toVariant();
    QVariantMap map = var.toMap();
    muunto_ = QJsonDocument::fromJson( kp()->asetus("PalkkaFiTilit").toUtf8() ).toVariant().toMap();
}

QVariantMap PalkkaFiTuonti::tuo(const QByteArray &data)
{
    PalkkaFiTuonti tuonti;

    QString utf8 = QString::fromUtf8(data);
    QStringList rivit = utf8.split('\n');

    if( rivit.length() < 2)
        return QVariantMap();

    QStringList ekarivi = rivit.takeFirst().split(";");
    if( ekarivi.length() < 9)
        return QVariantMap();

    QDate pvm = QDate::fromString(ekarivi.at(6), "dd.MM.yyyy");
    QString otsikko = kp()->kaanna("Palkat %1 - %2", kp()->asetus("kieli"))
            .arg(ekarivi.at(5))
            .arg(ekarivi.at(6));

    QVariantMap map;

    QVariantList viennit;

    while(rivit.length()) {
        QVariantMap tuotu = tuonti.tuoRivi(rivit.takeFirst(), pvm, otsikko);
        if(!tuotu.isEmpty())
            viennit.append(tuotu);
    }
    map.insert("tyyppi", TositeTyyppi::TUONTI);
    map.insert("pvm", pvm);
    map.insert("otsikko", otsikko);
    map.insert("viennit", viennit);
    return map;
}

const TositeVienti PalkkaFiTuonti::tuoRivi(const QString &rivi, const QDate& pvm, const QString& otsikko)
{
    TositeVienti vienti;

    QStringList kentat = rivi.split(";");

    if( kentat.length() < 5 || kentat.first() != "V")
        return QVariantMap();

    int vanhatili = kentat.at(1).toInt();
    int uusitili = muunto_.value(QString::number(vanhatili),QString::number(vanhatili)).toInt();
    vienti.setTili(uusitili);

    vienti.setPvm(pvm);
    vienti.setSelite(otsikko + " " + kentat.value(2));

    QString euro = kentat.at(3);
    euro.replace(QChar(','),QChar('.'));
    double eurot = euro.toDouble();

    if( eurot > 1e-5)
        vienti.setDebet(eurot);
    else if( eurot < -1e-5)
        vienti.setKredit(0-eurot);

    return vienti;
}
