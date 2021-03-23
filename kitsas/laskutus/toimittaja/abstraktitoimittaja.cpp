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
#include "abstraktitoimittaja.h"
#include "db/kirjanpito.h"
#include "laskuntoimittaja.h"

AbstraktiToimittaja::AbstraktiToimittaja(QObject *parent) : QObject(parent)
{
    connect(&timer_, &QTimer::timeout, this, &AbstraktiToimittaja::tarkastaJono);
}

AbstraktiToimittaja::~AbstraktiToimittaja()
{

}

void AbstraktiToimittaja::lisaaLasku(const QVariantMap &tosite)
{
    jono_.enqueue(tosite);
    if( jono_.size() == 1)
        timer_.start(250);
}

void AbstraktiToimittaja::keskeyta()
{
    jono_.clear();
}

void AbstraktiToimittaja::merkkaaToimitetuksi()
{
    merkkausjono_.enqueue( tositeMap().value("id").toInt() );
    if(!jono_.isEmpty())
        jono_.dequeue();
    tarkastaJono();

    if( !merkkausKaynnissa_ && !merkkausjono_.isEmpty())
        merkkaaJonosta();
}

void AbstraktiToimittaja::valmis()
{
    if(!jono_.isEmpty())
        jono_.dequeue();
    emit toimitettu();
    tarkastaJono();
}

void AbstraktiToimittaja::virhe(const QString &kuvaus)
{
    if(!jono_.isEmpty())
        jono_.dequeue();
    emit epaonnistui(kuvaus);
    tarkastaJono();
}

void AbstraktiToimittaja::peru()
{
    jono_.clear();
    LaskunToimittaja* laskunt = qobject_cast<LaskunToimittaja*>(parent());
    if( laskunt )
        laskunt->peru();
}

void AbstraktiToimittaja::tarkastaJono()
{
    if( !jono_.isEmpty()) {
        toimita();
    }
}

void AbstraktiToimittaja::merkkaaJonosta()
{
    merkkausKaynnissa_ = true;
    KpKysely *kysely = kpk(QString("/tositteet/%1").arg(merkkausjono_.dequeue()), KpKysely::PATCH);
    QVariantMap map;
    map.insert("tila", Tosite::LAHETETTYLASKU);
    connect( kysely, &KpKysely::vastaus, this, &AbstraktiToimittaja::merkattu);
    connect( kysely, &KpKysely::virhe, [this] { emit this->epaonnistui(tr("Tositteen päivittäminen epäonnistui")); } );
    kysely->kysy(map);
}

void AbstraktiToimittaja::merkattu()
{
    merkkausKaynnissa_ = false;
    emit toimitettu();
    if( !merkkausjono_.isEmpty())
        merkkaaJonosta();
}
