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
#include <QApplication>
#include <QSettings>

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
    QStringList systeemiKielet = QLocale::system().uiLanguages();
    QString systeemiKieli = systeemiKielet.value(0).contains("sv") ? "sv" : "fi";
    QSettings settings;
    QString valintaKieli = settings.value("uiKieli").toString();

    valitseUiKieli(valintaKieli.isEmpty() ? systeemiKieli : valintaKieli );

    qApp->installTranslator(&appTranslator_);
    qApp->installTranslator(&qtTranslator_);
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

Kielet::~Kielet()
{

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
    if( kieli.isEmpty() && kieliKoodit().contains(uiKieli())) {
            nykykieli_ = uiKieli_;
    } else if ( kieliKoodit().contains(kieli)) {
        nykykieli_ = kieli;
    } else {
        nykykieli_ = kieliKoodit().value(0);
    }

    emit kieliVaihtui(nykykieli_);
}

void Kielet::valitseUiKieli(const QString &kieli)
{
    uiKieli_ = kieli;
    appTranslator_.load("kitsas_" + kieli + ".qm", ":/tr/");
    qtTranslator_.load("qt_" + kieli + ".qm", ":/tr/");

    qDebug() << "AppTranslator " << appTranslator_.isEmpty();
    qDebug() << "QtTranslator " << qtTranslator_.isEmpty();

    QSettings settings;
    settings.setValue("uiKieli", kieli);
}

QString Kielet::kaanna(const QString &avain, const QString &kieli) const
{
    const QMap<QString,QString> map  = kaannokset_.value(avain);
    if( map.contains(kieli))
        return map.value(kieli);
    if( map.contains(nykykieli_))
        return map.value(nykykieli_);
    return avain;
}

QList<Kieli> Kielet::kielet() const
{
    QList<Kieli> palautettava;
    for(auto const & kieli : kielet_) {
        palautettava.append(Kieli(kieli.first, kieli.second.teksti()) );
    }
    return palautettava;
}

QStringList Kielet::kieliKoodit() const
{
    QStringList koodit;
    for(auto const & kieli : kielet_) {
        koodit.append(kieli.first);
    }
    return koodit;
}

QString Kielet::nykyinen() const
{
    return nykykieli_;
}

QString Kielet::uiKieli() const
{
    return uiKieli_;
}

Kielet* Kielet::instanssi__ = nullptr;

