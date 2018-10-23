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

#include "tositelaji.h"
#include "kirjanpito.h"

#include <QSqlQuery>

Tositelaji::Tositelaji() :
    id_(0), muokattu_(false)
{

}

Tositelaji::Tositelaji(int id, const QString& tunnus, const QString& nimi, const QByteArray& json) :
    id_(id), tunnus_(tunnus), nimi_(nimi), muokattu_(false)
{
    if( !json.isEmpty())
        json_.fromJson(json);
}

void Tositelaji::asetaId(int id)
{
    id_ = id;
}

void Tositelaji::asetaTunnus(const QString &tunnus)
{
    tunnus_ = tunnus;
    muokattu_ = true;
}

void Tositelaji::asetaNimi(const QString &nimi)
{
    nimi_ = nimi;
    muokattu_ = true;
}

void Tositelaji::nollaaMuokattu()
{
    muokattu_ = false;
}

int Tositelaji::montakoTositetta() const
{
    if( id() == 0)
        return 0;

    QSqlQuery kysely( QString("SELECT COUNT(id) FROM tosite WHERE laji=%1").arg( id()) );
    if( kysely.next())
        return kysely.value(0).toInt();

    return 0;
}

int Tositelaji::seuraavanTunnistenumero(const QDate pvm) const
{
    if( !kp()->tietokanta()->isOpen() )
        return 0;   // Model ei vielä käytössä

    if( !pvm.isValid() )    // Ei tunnistenumeroa jos ei päivää (maksuperusteinen lasku)
        return 0;

    Tilikausi kausi = kp()->tilikausiPaivalle( pvm );
    QString kysymys = QString("SELECT max(tunniste) FROM tosite WHERE "
                    " pvm BETWEEN \"%1\" AND \"%2\" "
                    " AND laji=\"%3\" ")
                                .arg(kausi.alkaa().toString(Qt::ISODate))
                                .arg(kausi.paattyy().toString(Qt::ISODate))
                                .arg( id() );

    QSqlQuery kysely;
    kysely.exec(kysymys);

    if( kysely.next())
        return kysely.value(0).toInt() + 1;
    else
        return 1;
}

