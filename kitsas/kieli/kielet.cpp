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
#include "kielet.h"

#include <QFile>
#include <QJsonDocument>

#include <QDebug>

Kielet::Kielet(const QString &tiedostonnimi)
{
    QFile file(tiedostonnimi);
    file.open(QIODevice::ReadOnly);
    QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
    QVariant var = doc.toVariant();
    QVariantMap map = var.toMap();

    QMapIterator<QString,QVariant> iter(map);
    while( iter.hasNext() ) {
        iter.next();
        kaannokset_.insert(iter.key(), QMap<QString,QString>());
        QMapIterator<QString,QVariant> kiter(iter.value().toMap());
        while( kiter.hasNext()) {
            kiter.next();
            kaannokset_[iter.key()].insert(kiter.key(), kiter.value().toString());
        }
    }
}

void Kielet::alustaKielet(const QString &kaannostiedostonnimi)
{
    instanssi__ = new Kielet(kaannostiedostonnimi);
    qInfo() << "Kielet alustettu " << kaannostiedostonnimi;
}

Kielet *Kielet::instanssi()
{
    return instanssi__;
}


void Kielet::asetaKielet(const QString &json)
{
    kielet_.clear();
    QVariantMap map = QJsonDocument::fromJson(json.toUtf8()).toVariant().toMap();
    QMapIterator<QString,QVariant> iter(map);
    while(iter.hasNext()) {
        iter.next();
        kielet_.append(qMakePair(iter.key(), Monikielinen(iter.value())));
    }
}

void Kielet::valitseKieli(const QString &kieli)
{
    nykykieli_ = kieli;
}

QString Kielet::kaanna(const QString &avain, const QString &kieli) const
{
    const QMap<QString,QString> map  = kaannokset_.value(avain);
    if( map.isEmpty())
        return avain;
    if(kieli.isEmpty())
        return map.value(nykykieli_);
    return map.value(kieli, avain);
}

QList<Kieli> Kielet::kielet() const
{
    QList<Kieli> palautettava;
    for(auto const & kieli : kielet_) {
        palautettava.append(Kieli(kieli.first, kieli.second.teksti()) );
    }
    return palautettava;
}

QString Kielet::nykyinen() const
{
    return nykykieli_;
}

Kielet* Kielet::instanssi__ = nullptr;

