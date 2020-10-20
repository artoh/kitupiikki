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
#include "yhteysmodel.h"

#include "db/kpkysely.h"
#include "db/kirjanpito.h"
#include "kierto/kiertomodel.h"

#include <QDebug>
#include <QSettings>
#include <QJsonDocument>
#include <QVariantList>

YhteysModel::YhteysModel(QObject *parent) :
    QAbstractListModel(parent)
{

}

void YhteysModel::alusta()
{
    KpKysely *initkysely = kysely("/init");
    connect( initkysely, &KpKysely::vastaus, this, &YhteysModel::initSaapuu );
    initkysely->kysy();
}

void YhteysModel::lataaInit(QVariant *reply)
{    

    QVariantMap map = reply->toMap();
    QMapIterator<QString, QVariant> iter( map );
    while (iter.hasNext()) {
        iter.next();
        QString avain = iter.key();

        if( avain == "asetukset")
            kp()->asetukset()->lataa( iter.value().toMap() );
        else if( avain == "tilit")
            kp()->tilit()->lataa( iter.value().toList() );
        else if( avain == "kohdennukset")
            kp()->kohdennukset()->lataa( iter.value().toList() );
        else if( avain == "tilikaudet")
            kp()->tilikaudet()->lataa( iter.value().toList() );
        else if( avain == "kierrot")
            kp()->kierrot()->lataa( iter.value().toList() );
        else if( avain == "tositesarjat") {
            QStringList lista;
            for(auto item : iter.value().toList())
                lista.append(item.toString());            
            QVariantMap map = QJsonDocument::fromJson( kp()->asetus("tositesarjat").toUtf8() ).toVariant().toMap();
            QVariantList values = map.values();
            for(auto value : values){
                QString laji = value.toString();
                if( !lista.contains(laji))
                    lista.append(laji);
            }

            kp()->asetaTositeSarjat(lista);
        }
    }

    // Pidetään yllä tietoa siitä, milloin viimeksi käytetty mitäkin
    // tilikarttaa, jotta tilikarttojen tilastointi toimii
    const QString& kartta = kp()->asetukset()->asetus("Tilikartta");
    if( !kartta.isEmpty()) {
        kp()->settings()->setValue("tilastokartta/" + kartta, QDate::currentDate());
    }
}

bool YhteysModel::onkoOikeutta(qlonglong oikeus)
{
    return oikeudet() & oikeus;
}


void YhteysModel::initSaapuu(QVariant *reply)
{
    lataaInit( reply );
    kp()->yhteysAvattu(this);
}
