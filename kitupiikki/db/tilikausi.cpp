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

#include <QSqlQuery>
#include <QVariant>
#include <QDebug>

#include "tilikausi.h"
#include "kirjanpito.h"
#include "asetusmodel.h"

Tilikausi::Tilikausi()
{

}

Tilikausi::Tilikausi(QVariantMap data) :
    KantaVariantti (data)
{
}

Tilikausi::Tilikausi(const QDate &alkaa, const QDate &paattyy)
{
    set("alkaa", alkaa);
    set("loppuu", paattyy);
}


QDateTime Tilikausi::arkistoitu()
{
    QString arkistoituna = str("arkisto");
    if( arkistoituna.isEmpty())
        return QDateTime();
    else
        return QDateTime::fromString( arkistoituna, Qt::ISODate);
}

QDateTime Tilikausi::viimeinenPaivitys() const
{
    QSqlQuery kysely( QString(R"(SELECT max(muokattu) FROM vienti WHERE pvm BETWEEN "%1" AND "%2" )").arg(alkaa().toString(Qt::ISODate)).arg(paattyy().toString(Qt::ISODate)));
    if( kysely.next() )
        return kysely.value(0).toDateTime();
    return QDateTime();
}

QString Tilikausi::kausivaliTekstina() const
{
    return QString("%1 - %2")
            .arg( alkaa().toString("dd.MM.yyyy"))
            .arg( paattyy().toString("dd.MM.yyyy"));
}

Tilikausi::TilinpaatosTila Tilikausi::tilinpaatoksenTila()
{
    if( paattyy() == kp()->asetukset()->pvm("TilinavausPvm") )
        return EILAADITATILINAVAUKSELLE;

    if( pvm("vahvistettu").isValid())
        return VAHVISTETTU;
    else if( !str("tilinpaatosteksti").isEmpty())
        return KESKEN;
    else
        return ALOITTAMATTA;
}


int Tilikausi::henkilosto()
{
    return luku("henkilosto");
}

QString Tilikausi::arkistoHakemistoNimi() const
{
    if( alkaa().month() == 1 && alkaa().day() == 1)
        return alkaa().toString("yyyy");
    else if( alkaa().day() == 1)
        return alkaa().toString("yyyy-MM");
    else
        return alkaa().toString("yyyy-MM-dd");
}

Tilikausi::Saannosto Tilikausi::pienuus()
{
    // HUOM! Ehdot ovat sentteinä!

    int mikroehdot = 0;
    int pienehdot = 0;

    if( tase() > 35000000)
        mikroehdot++;
    if( liikevaihto() > 70000000)
        mikroehdot++;
    if( henkilosto() > 10)
        mikroehdot++;

    if( tase() > 600000000)
        pienehdot++;
    if( liikevaihto() > 1200000000)
        pienehdot++;
    if( henkilosto() > 50)
        pienehdot++;

    if( mikroehdot <= 1)
        return MIKROYRITYS;
    else if(pienehdot <= 1)
        return PIENYRITYS;
    else
        return YRITYS;
}

int Tilikausi::pieniElinkeinonharjoittaja()
{
    int ehdot = 0;
    if( tase() > 10000000)
        ehdot++;
    if( liikevaihto() > 200000)
        ehdot++;
    if( henkilosto() > 3)
        ehdot++;

    return ehdot;
}

void Tilikausi::asetaKausitunnus(const QString &kausitunnus)
{
    kausitunnus_ = kausitunnus;
}

void Tilikausi::tallenna()
{
    QVariantMap map = data();
    map.remove("tase");
    map.remove("tulos");
    map.remove("liikevaihto");
    map.remove("viimeinen");

    KpKysely* kysely = kpk(QString("/tilikaudet/%1").arg( alkaa().toString(Qt::ISODate)), KpKysely::PUT );
    QObject::connect(kysely, &KpKysely::vastaus, kp()->tilikaudet(), &TilikausiModel::paivita);
    kysely->kysy( map );
}

void Tilikausi::poista()
{
    KpKysely *poisto = kpk(QString("/tilikaudet/%1").arg(alkaa().toString(Qt::ISODate)), KpKysely::DELETE);
    QObject::connect(poisto, &KpKysely::vastaus, kp()->tilikaudet(), &TilikausiModel::paivita);
    poisto->kysy();

}

