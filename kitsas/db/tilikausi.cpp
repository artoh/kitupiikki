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
#include <QSettings>

#include "tilikausi.h"
#include "kirjanpito.h"
#include "asetusmodel.h"

Tilikausi::Tilikausi()
{

}

Tilikausi::Tilikausi(QVariantMap data) :
    KantaVariantti (data)
{
    alkaa_ = data_.take("alkaa").toDate();
    paattyy_ = data_.take("loppuu").toDate();
}

Tilikausi::Tilikausi(const QDate &alkaa, const QDate &paattyy)
{
    alkaa_ = alkaa;
    paattyy_ = paattyy;
}


QDateTime Tilikausi::arkistoitu() const
{
    return kp()->settings()->value("arkistopvm/" + kp()->asetukset()->uid() + "-" + arkistoHakemistoNimi()).toDateTime();
}

QString Tilikausi::arkistopolku() const
{
    return kp()->settings()->value("arkistopolku/" + kp()->asetukset()->uid() + "-" + arkistoHakemistoNimi()).toString();
}

QString Tilikausi::uusiArkistopolku() const
{
    return kp()->settings()->value("arkistopolku/" + kp()->asetukset()->uid() ).toString() + "/" + arkistoHakemistoNimi();
}

QDateTime Tilikausi::viimeinenPaivitys() const
{
    QDateTime tositteet = arvo("paivitetty").toDateTime();
    QDateTime tpaatos = arvo("tilinpaatos").toDateTime();

    if( tpaatos.isValid() && tpaatos > tositteet)
        return tpaatos;
    return tositteet;
}

QString Tilikausi::kausivaliTekstina() const
{
    return QString("%1 - %2")
            .arg( alkaa().toString("dd.MM.yyyy"),
                paattyy().toString("dd.MM.yyyy"));
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

void Tilikausi::asetaAlkaa(const QDate &pvm)
{
    alkaa_ = pvm;
}

void Tilikausi::asetaPaattyy(const QDate &pvm)
{
    paattyy_ = pvm;
}

void Tilikausi::tallenna(const QDate &pvm)
{
    QVariantMap map = data();

    KpKysely* kysely = kpk(QString("/tilikaudet/%1").arg( pvm.isValid() ? pvm.toString(Qt::ISODate) : alkaa().toString(Qt::ISODate)), KpKysely::PUT );
    QObject::connect(kysely, &KpKysely::vastaus, kp()->tilikaudet(), &TilikausiModel::paivita);
    QObject::connect(kysely, &KpKysely::vastaus, kp(), &Kirjanpito::tilikausiAvattu);
    kysely->kysy( map );
}

void Tilikausi::poista()
{
    KpKysely *poisto = kpk(QString("/tilikaudet/%1").arg(alkaa().toString(Qt::ISODate)), KpKysely::DELETE);
    QObject::connect(poisto, &KpKysely::vastaus, kp()->tilikaudet(), &TilikausiModel::paivita);
    QObject::connect(poisto, &KpKysely::vastaus, kp(), &Kirjanpito::tilikausiAvattu);
    poisto->kysy();

}

QVariantMap Tilikausi::data() const
{
    QVariantMap map = KantaVariantti::data();
    map.insert("alkaa", alkaa_);
    map.insert("loppuu", paattyy_);

    map.remove("tase");
    map.remove("tulos");
    map.remove("liikevaihto");
    map.remove("viimeinen");
    return map;
}

bool Tilikausi::kuuluuko(const QDate &pvm) const
{
    return pvm >= alkaa_ && pvm <= paattyy_;
}

