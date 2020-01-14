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
#include "tulkki.h"
#include "db/kirjanpito.h"
#include <QFile>
#include <QJsonDocument>
#include <QVariantMap>

#include <QComboBox>

Tulkki::Tulkki(const QString &tiedostonnimi, QObject *parent)
    : QObject(parent)
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

QString Tulkki::k(const QString &avain, const QString &kieli) const
{
    QMap<QString,QString> map  = kaannokset_.value(avain);
    if( map.isEmpty())
        return avain;
    return map.value(kieli, avain);
}

void Tulkki::alustaKieliCombo(QComboBox *combo)
{
    combo->clear();
    lisaaKieli(combo, "fi", "suomi");
    lisaaKieli(combo, "sv", "ruotsi");
    lisaaKieli(combo, "en", "englanti");

    combo->setCurrentIndex( combo->findData( kp()->asetukset()->asetus("kieli") ) );

}

void Tulkki::lisaaKieli(QComboBox *combo, const QString &lyhenne, const QString &nimi)
{
    combo->addItem( QIcon(":/liput/" + lyhenne + ".png"), tulkkaa(nimi, kp()->asetukset()->asetus("kieli") ), lyhenne );
}
