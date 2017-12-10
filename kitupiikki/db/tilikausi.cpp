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

#include "tilikausi.h"
#include "kirjanpito.h"
#include "asetusmodel.h"

Tilikausi::Tilikausi()
{

}

Tilikausi::Tilikausi(QDate tkalkaa, QDate tkpaattyy, QByteArray json) :
    alkaa_(tkalkaa),
    paattyy_(tkpaattyy),
    json_(json)
{

}

QDateTime Tilikausi::arkistoitu()
{
    QString arkistoituna = json()->str("Arkistoitu");
    if( arkistoituna.isEmpty())
        return QDateTime();
    else
        return QDateTime::fromString( arkistoituna, Qt::ISODate);
}

void Tilikausi::merkitseNytArkistoiduksi(const QString &shatiiviste)
{
    QDateTime nyt = QDateTime( kp()->paivamaara(), QTime::currentTime());
    json_.set("Arkistoitu", nyt.toString(Qt::ISODate));
    json_.set("ArkistoSHA", shatiiviste);
}

QDateTime Tilikausi::viimeinenPaivitys() const
{
    QSqlQuery kysely( QString("SELECT max(muokattu) FROM vienti WHERE pvm BETWEEN \"%1\" AND \"%2\" ").arg(alkaa().toString(Qt::ISODate)).arg(paattyy().toString(Qt::ISODate)));
    if( kysely.next() )
        return kysely.value(0).toDateTime();
    return QDateTime();
}

QString Tilikausi::kausivaliTekstina() const
{
    return QString("%1 - %2")
            .arg( alkaa().toString(Qt::SystemLocaleShortDate))
            .arg( paattyy().toString(Qt::SystemLocaleShortDate));
}

Tilikausi::TilinpaatosTila Tilikausi::tilinpaatoksenTila()
{
    if( paattyy() == kp()->asetukset()->pvm("TilinavausPvm") )
        return EILAADITATILINAVAUKSELLE;

    QString tilateksti = json()->str("Tilinpaatos");
    if( tilateksti == "KESKEN")
        return KESKEN;
    else if( tilateksti == "VAHVISTETTU")
        return VAHVISTETTU;
    else
        return ALOITTAMATTA;
}

void Tilikausi::asetaTilinpaatostila(Tilikausi::TilinpaatosTila tila)
{
    if( tila == ALOITTAMATTA)
        json_.unset("Tilinpaatos");
    else if( tila == KESKEN)
        json_.set("Tilinpaatos","KESKEN");
    else if( tila == VAHVISTETTU )
        json_.set("Tilinpaatos","VAHVISTETTU");
}

int Tilikausi::tulos() const
{
    QSqlQuery kysely(  QString("SELECT SUM(kreditsnt), SUM(debetsnt) "
                               "FROM vienti, tili WHERE "
                               "pvm BETWEEN \"%1\" AND \"%2\" "
                               "AND vienti.tili=tili.id AND "
                               "tili.ysiluku > 300000000")
                       .arg(alkaa().toString(Qt::ISODate))
                       .arg(paattyy().toString(Qt::ISODate)));
    if( kysely.next())
        return kysely.value(0).toInt() - kysely.value(1).toInt();
    else
        return 0;
}

int Tilikausi::liikevaihto() const
{
    QSqlQuery kysely(  QString("SELECT SUM(kreditsnt), SUM(debetsnt) "
                               "FROM vienti, tili WHERE "
                               "pvm BETWEEN \"%1\" AND \"%2\" "
                               "AND vienti.tili=tili.id AND "
                               "tili.laji = \"CL\"")
                       .arg(alkaa().toString(Qt::ISODate))
                       .arg(paattyy().toString(Qt::ISODate)));
    if( kysely.next())
        return kysely.value(0).toInt() - kysely.value(1).toInt();
    else
        return 0;
}

int Tilikausi::tase() const
{
    QSqlQuery kysely(  QString("SELECT SUM(kreditsnt), SUM(debetsnt) "
                               "FROM vienti, tili WHERE "
                               "pvm <= \"%1\" "
                               "AND vienti.tili=tili.id AND "
                               "tili.ysiluku < 200000000")
                       .arg(paattyy().toString(Qt::ISODate)));
    if( kysely.next())
        return kysely.value(1).toInt() - kysely.value(0).toInt();
    else
        return 0;
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
