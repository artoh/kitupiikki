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
#include "saldotroute.h"

#include "db/kirjanpito.h"

#include <QDebug>

SaldotRoute::SaldotRoute(SQLiteModel* model) :
    SQLiteRoute(model, "/saldot")
{

}

QVariant SaldotRoute::get(const QString &/*polku*/, const QUrlQuery &urlquery)
{
    QVariantMap saldot;
    QDate pvm = QDate::fromString(urlquery.queryItemValue("pvm"),Qt::ISODate);
    Tilikausi kausi = kp()->tilikaudet()->tilikausiPaivalle(pvm);

    QSqlQuery kysely(db());

    if( !urlquery.hasQueryItem("tuloslaskelma")) {
        QString kysymys = "SELECT tili, sum(debet), sum(kredit) FROM Vienti WHERE PVM ";
        kysymys += urlquery.hasQueryItem("alkusaldot") ? "<" : "<=";
        kysymys += QString("'%1' ").arg(pvm.toString(Qt::ISODate));
        if( urlquery.hasQueryItem("tili"))
            kysymys += QString(" AND tili=%1 ").arg(urlquery.queryItemValue("tili").toInt());
        kysymys += " AND CAST(tili as text) < 3 GROUP BY tili ORDER BY tili ";

        kysely.exec(kysymys);
        while (kysely.next()) {
            QString tilistr = kysely.value(0).toString();
            if( tilistr.startsWith(QChar('1')))
                saldot.insert( tilistr, kysely.value(1).toDouble() - kysely.value(2).toDouble() );
            else
                saldot.insert( tilistr, kysely.value(2).toDouble() - kysely.value(1).toDouble() );
        }

        // Edellisten tulos
        kysely.exec(QString("SELECT sum(kredit), sum(debet) FROM Vienti WHERE CAST(tili as text) >= '3' "
                            "AND pvm<'%1'").arg(kausi.alkaa().toString(Qt::ISODate)));
        if( kysely.next()) {
            QString edtili = QString::number( kp()->tilit()->tiliTyypilla(TiliLaji::EDELLISTENTULOS).numero() ) ;
            double saldo = saldot.value(edtili).toDouble() + kysely.value(0).toDouble() - kysely.value(1).toDouble();
            if( qAbs(saldo) > 1e-5)
                saldot[edtili] = saldo;
        }
        // Nykyisen tulos
        if( !urlquery.hasQueryItem("alkusaldot") ) {
            kysely.exec(QString("SELECT sum(kredit), sum(debet) FROM Vienti WHERE CAST(tili as text) >= '3' "
                                "AND pvm BETWEEN '%1' AND '%2'")
                        .arg(kausi.alkaa().toString(Qt::ISODate))
                        .arg(pvm.toString(Qt::ISODate)));
            if( kysely.next()) {
                QString tulostili = QString::number( kp()->tilit()->tiliTyypilla(TiliLaji::KAUDENTULOS).numero() ) ;
                saldot.insert(tulostili, kysely.value(0).toDouble() - kysely.value(1).toDouble());
            }
        }
    }

    if( !urlquery.hasQueryItem("tase")) {
        QDate kaudenalku = kausi.alkaa();
        if( urlquery.hasQueryItem("alkupvm"))
            kaudenalku = QDate::fromString( urlquery.queryItemValue("alkupvm"), Qt::ISODate );

        QString kysymys("SELECT tili, SUM(kredit), SUM(debet) FROM Vienti WHERE pvm ");
        if( urlquery.hasQueryItem("alkusaldot"))
            kysymys += "<";
        else
            kysymys += "<=";
        kysymys += QString(" '%1' ").arg(pvm.toString(Qt::ISODate));
        if( urlquery.hasQueryItem("alkupvm"))
        if( urlquery.hasQueryItem("tili"))
            kysymys += QString(" AND tili=%1 ").arg(urlquery.queryItemValue("tili"));
        if( urlquery.hasQueryItem("kohdennus"))
            kysymys += QString(" AND kohdennus=%1 ").arg(urlquery.queryItemValue("kohdennus"));
        kysymys += QString(" AND pvm >= '%1' AND CAST(tili as text) >= 3 GROUP BY tili ORDER BY tili")
                .arg(kaudenalku.toString(Qt::ISODate));
        if( !kysely.exec(kysymys) )
            throw SQLiteVirhe(kysely);

        qDebug() << kysymys;


        while( kysely.next()) {
            saldot.insert( kysely.value(0).toString(), kysely.value(1).toDouble() - kysely.value(2).toDouble() );
        }
    }
    return saldot;
}
