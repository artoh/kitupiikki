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
#include "edellinenseuraavatieto.h"

#include "db/tositemodel.h"
#include "db/kirjanpito.h"

#include <QSqlQuery>

EdellinenSeuraavaTieto::EdellinenSeuraavaTieto(TositeModel *model, QObject *parent) :
    QObject(parent), model_(model)
{

}

void EdellinenSeuraavaTieto::paivita()
{
    edellinenId_ = -1;
    seuraavaId_ = -1;

    if( model_->tunniste() )
    {
        // Haetaan kaikki kyseisen tilikauden tositteet järjestykseen

        QSqlQuery kysely;
        Tilikausi nykykausi = kp()->tilikaudet()->tilikausiPaivalle(model_->pvm());

        if( kp()->asetukset()->onko("Samaansarjaan") )
            kysely.exec( QString("SELECT id FROM tosite WHERE pvm BETWEEN '%1' and '%2' ORDER BY tunniste")
                         .arg( nykykausi.alkaa().toString(Qt::ISODate) ).arg( nykykausi.paattyy().toString(Qt::ISODate)) );
        else
            kysely.exec( QString("SELECT tosite.id FROM tosite JOIN tositelaji ON tosite.laji=tositelaji.id "
                                 "WHERE pvm BETWEEN '%1' AND '%2' "
                                 "ORDER BY tositelaji.tunnus,tosite.tunniste")
                         .arg( nykykausi.alkaa().toString(Qt::ISODate) ).arg( nykykausi.paattyy().toString(Qt::ISODate)) );

        int edellinen = -1;

        while( kysely.next() )
        {
            if( kysely.value(0).toInt() == model_->id())
            {
                edellinenId_ = edellinen;
                if( kysely.next())
                    seuraavaId_ = kysely.value(0).toInt();
                break;
            }
            edellinen = kysely.value(0).toInt();
        }
    }

    // Lopuksi ilmoitukset
    emit edellinenOlemassa( edellinenId_ > -1);
    emit seuraavaOlemassa( seuraavaId_ > -1);
}
