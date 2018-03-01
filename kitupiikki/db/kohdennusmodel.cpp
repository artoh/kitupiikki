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
#include <QIcon>

#include <QDebug>
#include <QSqlError>

#include "kohdennusmodel.h"
#include "db/kirjanpito.h"
#include "db/tilikausi.h"




KohdennusModel::KohdennusModel(QSqlDatabase *tietokanta, QObject *parent) :
    QAbstractTableModel(parent), tietokanta_(tietokanta)
{

}

int KohdennusModel::rowCount(const QModelIndex & /* parent */) const
{
    return kohdennukset_.count();
}

int KohdennusModel::columnCount(const QModelIndex & /* parent */) const
{
    return 3;
}

QVariant KohdennusModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if( role == Qt::TextAlignmentRole)
        return QVariant( Qt::AlignCenter | Qt::AlignVCenter);
    else if( role != Qt::DisplayRole )
        return QVariant();
    else if( orientation == Qt::Horizontal)
    {
        switch (section)
        {
            case NIMI:
                return QVariant("Nimi");
            case ALKAA :
                return QVariant("Alkaa");
            case PAATTYY:
                return QVariant("Päättyy");
        }

    }
    return QVariant();
}

QVariant KohdennusModel::data(const QModelIndex &index, int role) const
{
    if( !index.isValid())
        return QVariant();

    Kohdennus kohdennus = kohdennukset_[index.row()];

    if( role == IdRooli)
        return QVariant( kohdennus.id() );
    else if( role == TyyppiRooli)
        return QVariant( kohdennus.tyyppi() );
    else if( role == NimiRooli )
        return kohdennus.nimi();
    else if( role == AlkaaRooli)
        return kohdennus.alkaa();
    else if( role == PaattyyRooli)
        return kohdennus.paattyy();
    else if( role == VientejaRooli)
        return kohdennus.montakoVientia();

    else if( role == Qt::TextAlignmentRole)
        return QVariant( Qt::AlignLeft | Qt::AlignVCenter);
    else if( role == Qt::DisplayRole || role == Qt::EditRole)
    {
        switch( index.column())
        {
            case NIMI: return QVariant( kohdennus.nimi() );
            case ALKAA: return QVariant(kohdennus.alkaa() );
            case PAATTYY: return QVariant(kohdennus.paattyy());
        }
    }
    else if( role == Qt::DecorationRole && index.column() == NIMI)
    {
        if( kohdennus.tyyppi() == Kohdennus::PROJEKTI)
            return QIcon(":/pic/projekti.png");
        else if( kohdennus.tyyppi() == Kohdennus::KUSTANNUSPAIKKA)
            return QIcon(":/pic/kohdennus.png");
    }

    return QVariant();
}


bool KohdennusModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
    if( role == TyyppiRooli)
    {
        if( value.toInt() == Kohdennus::PROJEKTI)
            kohdennukset_[index.row()].asetaTyyppi( Kohdennus::PROJEKTI);
        else if( value.toInt() == Kohdennus::KUSTANNUSPAIKKA)
            kohdennukset_[index.row()].asetaTyyppi( Kohdennus::KUSTANNUSPAIKKA);
    }
    else if( role == NimiRooli)
    {
        kohdennukset_[ index.row()].asetaNimi( value.toString());
    }
    else if( role == AlkaaRooli)
    {
        kohdennukset_[index.row()].asetaAlkaa( value.toDate());
    }
    else if( role == PaattyyRooli)
    {
        kohdennukset_[index.row()].asetaPaattyy( value.toDate());
    }
    else
        return false;

    return true;
}

QString KohdennusModel::nimi(int id) const
{
    return kohdennus(id).nimi();
}

Kohdennus KohdennusModel::kohdennus(int id) const
{
    foreach (Kohdennus projekti, kohdennukset_)
    {
        if( projekti.id() == id)
            return projekti;
    }
    return Kohdennus();
}

Kohdennus KohdennusModel::kohdennus(const QString &nimi) const
{
    foreach (Kohdennus projekti, kohdennukset_)
    {
        if( projekti.nimi() == nimi)
            return projekti;
    }
    return Kohdennus();
}

QList<Kohdennus> KohdennusModel::kohdennukset() const
{
    return kohdennukset_;
}

void KohdennusModel::poistaRivi(int riviIndeksi)
{
    Kohdennus kohdennus = kohdennukset_[riviIndeksi];
    if( kohdennus.montakoVientia() )
        return;         // Ei voi poistaa, jos kirjauksia

    beginRemoveRows( QModelIndex(), riviIndeksi, riviIndeksi);
    if( kohdennus.id() )
        poistetutIdt_.append( kohdennus.id());

    kohdennukset_.removeAt(riviIndeksi);
    endRemoveRows();
}

bool KohdennusModel::onkoMuokattu() const
{
    if( poistetutIdt_.count())
        return true;
    foreach (Kohdennus kohdennus, kohdennukset_)
    {
        if( kohdennus.muokattu())
            return true;
    }
    return false;
}

void KohdennusModel::lataa()
{
    beginResetModel();
    kohdennukset_.clear();
    QSqlQuery kysely(*tietokanta_);

    kysely.exec("select id, tyyppi, nimi, alkaa, loppuu FROM kohdennus");
    while( kysely.next() )
    {
        kohdennukset_.append( Kohdennus( kysely.value(0).toInt(),
                                     kysely.value(1).toInt(),
                                     kysely.value(2).toString(),
                                     kysely.value(3).toDate(),
                                     kysely.value(4).toDate()));
    }
    endResetModel();
}

void KohdennusModel::lisaaUusi(Kohdennus uusi)
{
    beginInsertRows(QModelIndex(), kohdennukset_.count(), kohdennukset_.count());
    kohdennukset_.append( uusi );
    endInsertRows();
}

void KohdennusModel::tallenna()
{
    tietokanta_->transaction();
    QSqlQuery kysely(*tietokanta_);

    for(int i=0; i < kohdennukset_.count(); i++)
    {
        Kohdennus kohdennus = kohdennukset_[i];
        if( kohdennus.muokattu())
        {
            if( kohdennus.id())
            {
                // Muokkaus
                kysely.prepare("UPDATE kohdennus SET nimi=:nimi, tyyppi=:tyyppi,"
                               "alkaa=:alkaa, loppuu=:loppuu WHERE id=:id");
                kysely.bindValue(":id", kohdennus.id());
            }
            else
            {
                kysely.prepare("INSERT INTO kohdennus(tyyppi, nimi, alkaa, loppuu) "
                               "VALUES(:tyyppi, :nimi, :alkaa, :loppuu) ");
            }
            kysely.bindValue(":nimi", kohdennus.nimi());
            kysely.bindValue(":tyyppi", kohdennus.tyyppi());
            kysely.bindValue(":alkaa", kohdennus.alkaa());
            kysely.bindValue(":loppuu", kohdennus.paattyy());

            if( kysely.exec() )
                kohdennukset_[i].nollaaMuokattu();

            if( !kohdennus.id() )
                kohdennukset_[i].asetaId( kysely.lastInsertId().toInt());

            qDebug() << kysely.lastQuery() << " " << kysely.lastError().text() << " " << kohdennus.nimi() << "  "
                     << kysely.lastInsertId().toInt();
        }
    }
    foreach (int id, poistetutIdt_)
    {
        kysely.exec( QString("DELETE FROM kohdennus WHERE id=%1").arg(id));
    }
    poistetutIdt_.clear();

    tietokanta_->commit();
}



