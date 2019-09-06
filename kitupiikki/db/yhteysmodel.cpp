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

#include <QDebug>

YhteysModel::YhteysModel(QObject *parent)
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
    QMapIterator<QString, QVariant> iter( reply->toMap() );
    while (iter.hasNext()) {
        iter.next();
        QString avain = iter.key();

        if( avain == "asetukset")
            kp()->asetukset()->lataa( iter.value().toMap() );
        else if( avain == "tilit")
            kp()->tilit()->lataa( iter.value().toList() );
        else if( avain == "kohdennukset")
            kp()->kohdennukset()->lataa( iter.value().toList() );
        else if( avain == "tositelajit")
            kp()->tositelajit()->lataa( iter.value().toList() );
        else if( avain == "tilikaudet")
            kp()->tilikaudet()->lataa( iter.value().toList() );
    }
}


void YhteysModel::initSaapuu(QVariant *reply)
{
    qDebug() << "INIT " << reply;

    lataaInit( reply );
    kp()->yhteysAvattu(this);
}
