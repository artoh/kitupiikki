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

#include "tositelajimodel.h"
#include "db/kirjanpito.h"

#include <QSqlQuery>
#include <QJsonDocument>

#include <QDebug>
#include <QSqlError>

TositelajiModel::TositelajiModel(QSqlDatabase *tietokanta, QObject *parent)
    : QAbstractTableModel(parent), tietokanta_(tietokanta)
{

}

int TositelajiModel::rowCount(const QModelIndex & /* parent */ ) const
{
    return lajit_.count();

}

int TositelajiModel::columnCount(const QModelIndex & /* parent */) const
{
    return 3;
}

QVariant TositelajiModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if( role == Qt::TextAlignmentRole)
        return QVariant( Qt::AlignCenter | Qt::AlignVCenter);
    else if( orientation == Qt::Horizontal && role == Qt::DisplayRole )
    {
        switch (section)
        {
        case TUNNUS :
            return QVariant("Tunnus");
        case NIMI:
            return QVariant("Nimi");
        case VASTATILI:
            return QVariant("Vastatili");
        }
    }
    return QVariant();
}

QVariant TositelajiModel::data(const QModelIndex &index, int role) const
{
    if( !index.isValid())
        return QVariant();

    Tositelaji laji = lajit_[index.row()];

    if( role == IdRooli)
        return QVariant( laji.id() );
    else if( role == TunnusRooli)
        return QVariant( laji.tunnus());
    else if( role == NimiRooli)
        return QVariant( laji.nimi());
    else if( role == VastatiliNroRooli)
        return QVariant( laji.json()->luku("Vastatili"));
    else if( role == TositeMaaraRooli)
        return laji.montakoTositetta();
    else if( role == KirjausTyyppiRooli)
        return QVariant( laji.json()->luku("Kirjaustyyppi"));
    else if( role == OletustiliRooli)
        return QVariant( laji.json()->luku("Oletustili"));
    else if( role == JsonRooli)
         return QVariant( laji.json()->toJson());

    else if( role == Qt::DisplayRole || role == Qt::EditRole)
    {
        switch (index.column())
        {
            case TUNNUS: return QVariant(laji.tunnus() );

            case NIMI:
                    return QVariant( laji.nimi() );
            case VASTATILI:
                // json talletetaan vastatilin tilinumero
                int tilinro = laji.json()->luku("Vastatili");
                if( tilinro)
                {
                    Tili tili = kp()->tilit()->tiliNumerollaVanha(tilinro);
                    return QVariant( QString("%1 %2").arg(tili.numero()).arg(tili.nimi()) );
                }
                return QVariant();
        }
    }
    else if( role == Qt::TextAlignmentRole)
    {
        return QVariant( Qt::AlignLeft | Qt::AlignVCenter);
    }
    else if( role == Qt::DecorationRole && index.column() == NIMI)
    {
        if( laji.id()==0)
            return QIcon(":/pic/Possu64.png");

        switch ( laji.json()->luku("Kirjaustyyppi")) {
            case OSTOLASKUT : return QIcon(":/pic/poista.png");
            case MYYNTILASKUT: return QIcon(":/pic/lisaa.png");
        }
        int tilinro = laji.json()->luku("Vastatili");
        if( tilinro)
        {
            Tili tili = kp()->tilit()->tiliNumerollaVanha(tilinro);
            if( tili.onko(TiliLaji::KATEINEN))
                return QIcon(":/pic/rahaa.png");
            else if(tili.onko(TiliLaji::PANKKITILI) && laji.json()->luku("Kirjaustyyppi")==TILIOTE)
                return QIcon(":/pic/tekstisivu.png");
        }



        return QIcon(":/pic/tyhja.png");
    }

    return QVariant();
}


bool TositelajiModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
    if( role == Qt::EditRole)
    {
        switch (index.column()) {
        case TUNNUS:
            lajit_[ index.row()].asetaTunnus( value.toString());
            return true;
        case NIMI:
            lajit_[ index.row()].asetaNimi(value.toString());
            return true;
        case VASTATILI:
            if( value.toInt())
                lajit_[index.row()].json()->set("Vastatili", value.toInt());
            else
                lajit_[index.row()].json()->unset("Vastatili");
        default:
            ;
        }
    }
    else if( role == KirjausTyyppiRooli )
    {
        if( value.toInt())
            lajit_[index.row()].json()->set("Kirjaustyyppi", value.toInt());
        else
            lajit_[index.row()].json()->unset("Kirjaustyyppi");
    }
    else if( role == OletustiliRooli)
        lajit_[index.row()].json()->set("Oletustili", value.toInt());
    else if( role == TunnusRooli )
        lajit_[index.row()].asetaTunnus( value.toString());
    else if( role == NimiRooli )
        lajit_[index.row()].asetaNimi( value.toString());
    else if( role == JsonRooli )
        lajit_[index.row()].json()->fromJson( value.toByteArray());
    else if( role == VastatiliNroRooli )
    {
        if( value.toInt())
            lajit_[index.row()].json()->set("Vastatili", value.toInt());
        else
            lajit_[index.row()].json()->unset("Vastatili");
    }

    return false;
}

bool TositelajiModel::onkoMuokattu() const
{
    if( poistetutIdt_.count())
        return true;
    foreach (Tositelaji laji, lajit_)
    {
        if( laji.muokattu())
            return true;
    }
    return false;
}

void TositelajiModel::poistaRivi(int riviIndeksi)
{
    Tositelaji laji = lajit_[riviIndeksi];

    if( laji.montakoTositetta() || laji.id() < 2)
        return;     // Käytettyjä tai suojattuja ei voi poistaa!

    beginRemoveRows( QModelIndex(), riviIndeksi, riviIndeksi);
    if( laji.id())
        poistetutIdt_.append( laji.id());
    lajit_.removeAt( riviIndeksi);
    endRemoveRows();
}

Tositelaji TositelajiModel::tositelaji(int id) const
{

    foreach (Tositelaji laji, lajit_)
    {
        if( laji.id() == id)
            return laji;
    }
    return Tositelaji();
}

QModelIndex TositelajiModel::lisaaRivi()
{
    beginInsertRows( QModelIndex(), lajit_.count(), lajit_.count() );
    lajit_.append( Tositelaji() );
    endInsertRows();
    return index( lajit_.count()-1, 0);

}

void TositelajiModel::lataa(const QVariantList &lista)
{
    beginResetModel();
    lajit_.clear();

    for(QVariant item : lista )
    {
        QVariantMap map = item.toMap();
        int id = map.take("id").toInt();
        QString tunnus = map.take("tunnus").toString();
        QString nimi = map.take("nimi").toString();

        QJsonDocument doc = QJsonDocument::fromVariant(map);

        lajit_.append( Tositelaji(id, tunnus, nimi, doc.toJson()) );
    }

    qDebug() << "Ladattu " << lista.count() << " tositelajia ";

    endResetModel();
}

void TositelajiModel::lataa()
{
    beginResetModel();

    lajit_.clear();
    QSqlQuery kysely(*tietokanta_);
    kysely.exec("SELECT id,tunnus,nimi,json FROM tositelaji ORDER BY id");
    while( kysely.next())
    {
        lajit_.append( Tositelaji(kysely.value(0).toInt(), kysely.value(1).toString(),
                                      kysely.value(2).toString(), kysely.value(3).toByteArray() ));
    }

    endResetModel();
}

bool TositelajiModel::tallenna()
{
    QSqlQuery tallennus( *tietokanta_);
    for(int i=0; i < lajit_.count(); i++)
    {

        if( lajit_[i].muokattu() )
        {
            if( lajit_[i].id() )
            {
                tallennus.prepare("UPDATE tositelaji SET tunnus=:tunnus, nimi=:nimi, json=:json WHERE _rowid_=:id");
                tallennus.bindValue(":id", lajit_[i].id());
            }
            else
            {
                tallennus.prepare("INSERT INTO tositelaji(tunnus,nimi,json) VALUES(:tunnus,:nimi,:json)");
            }
            tallennus.bindValue(":tunnus", lajit_[i].tunnus() );
            tallennus.bindValue(":nimi", lajit_[i].nimi() );
            tallennus.bindValue(":json", lajit_[i].json()->toSqlJson() );

            if(tallennus.exec())
                lajit_[i].nollaaMuokattu();

            if( !lajit_[i].id())
                lajit_[i].asetaId( tallennus.lastInsertId().toInt());

        }

    }
    foreach (int id, poistetutIdt_)
    {
        tallennus.exec( QString("DELETE tositelaji WHERE id=%1").arg(id));
    }
    poistetutIdt_.clear();

    return true;
}


