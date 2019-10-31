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
#include "viennitroute.h"
#include "tositeroute.h"

#include "model/tosite.h"

ViennitRoute::ViennitRoute(SQLiteModel* model) :
    SQLiteRoute(model,"/viennit")
{

}

QVariant ViennitRoute::get(const QString &polku, const QUrlQuery &urlquery)
{
    if( polku.toInt())
        return vienti(polku.toInt());

    QStringList ehdot;
    ehdot.append(QString("tila >= %1").arg(Tosite::KIRJANPIDOSSA));

    if( urlquery.hasQueryItem("alkupvm"))
        ehdot.append(QString("vienti.pvm >= '%1'").arg(urlquery.queryItemValue("alkupvm")));
    if( urlquery.hasQueryItem("loppupvm"))
        ehdot.append(QString("vienti.pvm <= '%1'").arg(urlquery.queryItemValue("loppupvm")));
    if( urlquery.hasQueryItem("tili"))
        ehdot.append(QString("tili=%1").arg(urlquery.queryItemValue("tili")));

    // TODO Kohdennus ja merkkaus

    QString jarjestys;
    if( urlquery.queryItemValue("jarjestys")=="tili")
        jarjestys = "Vienti.tili,";
    else if( urlquery.queryItemValue("jarjestys") == "tosite")
        jarjestys = "Tosite.sarja,Tosite.tunniste,";
    else if( urlquery.queryItemValue("jarjestys") == "laji")
        jarjestys = "Tosite.tyyppi,";
    else if( urlquery.queryItemValue("jarjestys")=="laji,tosite")
        jarjestys = "Tosite.tyyppi,Tosite.sarja,Tosite.tunniste,";


    QString kysymys("SELECT vienti.id AS id, vienti.pvm as pvm, vienti.tili as tili, debetsnt, kreditsnt, alvkoodi, alvprosentti, "
                    "selite, vienti.kohdennus as kohdennus, eraid as era_id, vienti.tosite as tosite_id, tosite.pvm as tosite_pvm, tosite.tunniste as tosite_tunniste,"
                    "tosite.tyyppi as tosite_tyyppi, tosite.sarja as tosite_sarja, kumppani.id as kumppani_id, "
                    "kumppani.nimi as kumppani_nimi, "
                    "CAST( (SELECT COUNT(liite.id) FROM Liite WHERE liite.tosite=tosite.id) AS int) AS liitteita "
                    "FROM Vienti JOIN Tosite ON Vienti.tosite=Tosite.id "
                    "LEFT OUTER JOIN Kumppani ON tosite.kumppani=kumppani.id "
                    "WHERE ");
    kysymys.append( ehdot.join(" AND ") );
    kysymys.append(" ORDER BY " + jarjestys + "Vienti.pvm, Vienti.id ");

    QSqlQuery kysely(db());
    kysely.exec(kysymys);

    QVariantList viennit = resultList(kysely);
    taydennaEratJaMerkkaukset(viennit);
    return viennit;

}

QVariant ViennitRoute::vienti(int id)
{
    QString kysymys(QString("SELECT vienti.id, pvm, tili, selite, debetsnt, kreditsnt, kumppani.nimi as kumppani "
                            "FROM Vienti LEFT OUTER JOIN Kumppani ON Vienti.kumppani=Kumppani.id WHERE Vienti.id=%1").arg(id));
    QSqlQuery kysely( db());
    kysely.exec(kysymys);

    return resultMap(kysely);
}
