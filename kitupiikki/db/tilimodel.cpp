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
#include <QSqlError>

#include <QDebug>

#include "tilimodel.h"
#include "tili.h"


TiliModel::TiliModel(QSqlDatabase *tietokanta, QObject *parent) :
    QAbstractTableModel(parent), tietokanta_(tietokanta)
{

}

int TiliModel::rowCount(const QModelIndex & /* parent */) const
{
    return tilit_.count();
}

int TiliModel::columnCount(const QModelIndex & /* parent */) const
{
    return 3;
}

QVariant TiliModel::data(const QModelIndex &index, int role) const
{
    if( !index.isValid())     
        return QVariant();

    Tili tili = tilit_.value(index.row());

    if( role == IdRooli )
        return QVariant( tili.id());
    else if( role == NroRooli )
        return QVariant( tili.numero());
    else if( role == NimiRooli )
        return QVariant( tili.nimi());
    else if( role == NroNimiRooli)
        return QVariant( QString("%1 %2").arg(tili.numero()).arg(tili.nimi()));
    else if( role == OtsikkotasoRooli)
        return QVariant( tili.otsikkotaso());
    else if( role == TyyppiRooli )
        return QVariant( tili.tyyppi());

    else if( role == Qt::DisplayRole || role == Qt::EditRole)
    {
        switch (index.column())
        {
        case NRONIMI :
            return QVariant( QString("%1 %2").arg(tili.numero()).arg(tili.nimi()));
        case NUMERO:
            return QVariant( tili.numero());
        case NIMI :
            return QVariant( tili.nimi());
        }
    }
    return QVariant();
}

void TiliModel::lisaaTili(Tili uusi)
{
    beginInsertRows( QModelIndex(), tilit_.count(), tilit_.count()  );
    tilit_.append(uusi);
    // TODO - lisätään oikeaan paikkaan kasiluvun mukaan
    endInsertRows();
}

Tili TiliModel::tiliIdlla(int id) const
{
    foreach (Tili tili, tilit_)
    {
        if( tili.id() == id)
            return tili;
    }
    return Tili();
}

Tili TiliModel::tiliNumerolla(int numero) const
{
    foreach (Tili tili, tilit_)
    {
        if( tili.numero() == numero && tili.otsikkotaso() == 0)
            return tili;
    }
    return Tili();
}

void TiliModel::lataa()
{
    beginResetModel();
    tilit_.clear();

    QSqlQuery kysely( *tietokanta_ );
    kysely.exec("SELECT id, nro, nimi, tyyppi, tila, json"
                "otsikkotaso FROM tili ORDER BY ysiluku");

    while(kysely.next())
    {
        Tili uusi( kysely.value(0).toInt(),     // id
                   kysely.value(1).toInt(),     // nro
                   kysely.value(2).toString(),  // nimi
                   kysely.value(3).toString(),  // tyyppi
                   kysely.value(4).toInt(),     // tila
                   kysely.value(5).toInt()      // otsikkotaso
                   );
        uusi.json()->fromJson( kysely.value(6).toByteArray());  // Luetaan json-kentät
        tilit_.append(uusi);
    }
    endResetModel();
}

void TiliModel::tallenna()
{
    QSqlQuery kysely(*tietokanta_);
    foreach (Tili tili, tilit_)
    {
        if( tili.onkoValidi() && tili.muokattu())
        {
            if( tili.id())
            {
                // Muokkaus
                kysely.prepare("UPDATE tili SET nro=:nro, nimi=:nimi, tyyppi=:tyyppi, "
                               "tila=:tila, otsikkotaso=:otsikkotaso, ysiluku=:ysiluku, json=:json "
                               "WHERE id=:id");
                kysely.bindValue(":id", tili.id());
            }
            else
            {
                // Tallennus
                kysely.prepare("INSERT INTO tili(nro, nimi, tyyppi, tila, otsikkotaso, ysiluku, json) "
                               "VALUES(:nro, :nimi, :tyyppi, :tila, :otsikkotaso, :ysiluku, :json) ");

            }
            kysely.bindValue(":nro", tili.numero());
            kysely.bindValue(":nimi", tili.nimi());
            kysely.bindValue(":tyyppi", tili.tyyppi());
            kysely.bindValue(":tila", tili.tila());
            kysely.bindValue(":otsikkotaso", tili.otsikkotaso());
            kysely.bindValue(":ysiluku", tili.ysivertailuluku());
            kysely.bindValue(":json", tili.json()->toSqlJson());

            if( kysely.exec() )
                tili.nollaaMuokattu();

            if( tili.id())
                tili.asetaId( kysely.lastInsertId().toInt() );

        }
    }
}
