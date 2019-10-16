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
#include <QMessageBox>
#include <QSqlError>

#include "kohdennusmodel.h"
#include "db/kirjanpito.h"
#include "db/tilikausi.h"




KohdennusModel::KohdennusModel(QObject *parent) :
    QAbstractTableModel(parent)
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
        return kohdennus.tyyppiKuvake();
    }

    return QVariant();
}



QString KohdennusModel::nimi(int id) const
{
    return kohdennus(id).nimi();
}

Kohdennus KohdennusModel::kohdennus(const int id) const
{
    foreach (Kohdennus projekti, kohdennukset_)
    {
        if( projekti.id() == id)
            return projekti;
    }

    qDebug() << "Kohdennusta ei löydy " << id;
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

QList<Kohdennus> KohdennusModel::vainKohdennukset(const QDate &pvm) const
{
    QList<Kohdennus> k;
    for( const Kohdennus& kohdennus : kohdennukset_)
        if( kohdennus.tyyppi() != Kohdennus::MERKKAUS &&
            !(kohdennus.alkaa() > pvm) &&
            !(kohdennus.paattyy() < pvm) )
            k.append(kohdennus);
    return k;
}

bool KohdennusModel::kohdennuksia() const
{
    for( const Kohdennus& kohdennus : kohdennukset_)
        if( kohdennus.tyyppi() == Kohdennus::KUSTANNUSPAIKKA ||
            kohdennus.tyyppi() == Kohdennus::PROJEKTI)
            return true;
    return false;
}

bool KohdennusModel::merkkauksia() const
{
    for( const Kohdennus& kohdennus : kohdennukset_)
        if( kohdennus.tyyppi() == Kohdennus::MERKKAUS)
            return true;
    return false;
}

void KohdennusModel::poistaRivi(int riviIndeksi)
{
    Kohdennus kohdennus = kohdennukset_[riviIndeksi];
    if( kohdennus.montakoVientia() )
        return;         // Ei voi poistaa, jos kirjauksia

    beginRemoveRows( QModelIndex(), riviIndeksi, riviIndeksi);

    KpKysely *kysely = kpk(QString("/kohdennukset/%1").arg(kohdennus.id()), KpKysely::DELETE);
    kysely->kysy();
    kohdennukset_.removeAt(riviIndeksi);

    endRemoveRows();
}


void KohdennusModel::lataa(QVariantList lista)
{
    beginResetModel();
    kohdennukset_.clear();
    for( QVariant item : lista )
    {
        QVariantMap map = item.toMap();
        kohdennukset_.append( Kohdennus( map ));
    }
    endResetModel();

    qDebug() << "Ladattu " << kohdennukset_.count() << " kohdennusta ";
}

Kohdennus *KohdennusModel::lisaa(int tyyppi)
{
    beginInsertRows(QModelIndex(), kohdennukset_.count(), kohdennukset_.count());
    kohdennukset_.append(Kohdennus(tyyppi));
    endInsertRows();
    return &kohdennukset_[ kohdennukset_.count()-1 ];
}

void KohdennusModel::tallenna(int indeksi)
{
    KpKysely *kysely = nullptr;
    const Kohdennus& kohdennus = kohdennukset_.at(indeksi);
    if( kohdennus.id())
        kysely = kpk( QString("/kohdennukset/%1").arg(kohdennus.id()), KpKysely::PUT);
    else
        kysely = kpk( "/kohdennukset", KpKysely::POST);
    connect( kysely, &KpKysely::vastaus,
             [this, indeksi] (QVariant* data) { this->tallennettu(indeksi, data);} );
    connect( kysely, &KpKysely::virhe,
             [](int, const QString& selitys) { QMessageBox::critical(nullptr,tr("Virhe tallentamisessa"), tr("Kohdennuksen tallentaminen epäonnistui.\n%1").arg(selitys)); } );
    kysely->kysy( kohdennus.data());
}


void KohdennusModel::paivita()
{
    KpKysely *kysely = kpk("/kohdennukset");
    connect( kysely, &KpKysely::vastaus, this, &KohdennusModel::lataaData );
    kysely->kysy();
}

void KohdennusModel::lataaData(const QVariant *lista)
{
    lataa(lista->toList());
}

void KohdennusModel::tallennettu(int indeksi, QVariant *data)
{
    if( !kohdennukset_.at(indeksi).id() ) {
        kohdennukset_[indeksi].asetaId( data->toMap().value("id").toInt() );
    }
    emit dataChanged( index(indeksi,0), index(indeksi,PAATTYY) );
}



