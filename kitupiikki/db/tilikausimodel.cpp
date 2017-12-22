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

#include "tilikausimodel.h"
#include "kirjanpito.h"

TilikausiModel::TilikausiModel(QSqlDatabase *tietokanta, QObject *parent) :
    QAbstractTableModel(parent), tietokanta_(tietokanta)
{

}

int TilikausiModel::rowCount(const QModelIndex & /* parent */) const
{
    return kaudet_.count();
}

int TilikausiModel::columnCount(const QModelIndex & /* parent */) const
{
    return 6;
}

QVariant TilikausiModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if( role == Qt::TextAlignmentRole)
        return QVariant( Qt::AlignCenter | Qt::AlignVCenter);

    else if( orientation == Qt::Horizontal && role == Qt::DisplayRole )
    {
        switch (section)
        {
        case KAUSI :
            return QVariant("Tilikausi");
        case LIIKEVAIHTO:
            return tr("Liikevaihto");
        case TASE:
            return tr("Tase");
        case TULOS:
            return QVariant("Yli/alijäämä");
        case ARKISTOITU:
            return QVariant("Arkistoitu");
        case TILINPAATOS:
            return QVariant("Tilinpäätös");
        }
    }
    return QVariant();

}

QVariant TilikausiModel::data(const QModelIndex &index, int role) const
{
    if( !index.isValid())
        return QVariant();

    Tilikausi kausi = kaudet_.value(index.row());

    if( role == Qt::DisplayRole)
    {
        if( index.column() == KAUSI)
            return QVariant( tr("%1 - %2")
                             .arg(kausi.alkaa().toString(Qt::SystemLocaleShortDate))
                             .arg(kausi.paattyy().toString(Qt::SystemLocaleShortDate)));
        else if( index.column() == TULOS)
            return QString("%L1 €").arg( kausi.tulos()  / 100.0,0,'f',2);
        else if(index.column() == LIIKEVAIHTO)
            return QString("%L1 €").arg( kausi.liikevaihto()  / 100.0,0,'f',2);
        else if(index.column() == TASE)
            return QString("%L1 €").arg( kausi.tase()  / 100.0,0,'f',2);

        else if( index.column() == ARKISTOITU )
            return kausi.arkistoitu().date();
        else if( index.column() == TILINPAATOS )
        {
            if( kausi.tilinpaatoksenTila() == Tilikausi::VAHVISTETTU)
                return tr("Vahvistettu");
            else if( kausi.tilinpaatoksenTila() == Tilikausi::KESKEN)
                return tr("Keskeneräinen");
            else if( kausi.paattyy().daysTo( kp()->paivamaara()) > 1 &&
                     kausi.paattyy().daysTo( kp()->paivamaara()) < 4 * 30 &&
                     kausi.tilinpaatoksenTila() != Tilikausi::EILAADITATILINAVAUKSELLE)
                return tr("Aika laatia!");
        }
    }
    else if( role == AlkaaRooli)
        return QVariant( kausi.alkaa());
    else if( role == PaattyyRooli )
        return QVariant( kausi.paattyy());
    else if( role == HenkilostoRooli )
        return kausi.json()->luku("Henkilosto");
    else if( role == Qt::TextAlignmentRole)
    {
        if( index.column()==TULOS )
            return QVariant(Qt::AlignRight | Qt::AlignVCenter);
        else
            return QVariant( Qt::AlignLeft | Qt::AlignVCenter);

    }
    else if( role == Qt::DecorationRole )
    {
        if( index.column() == KAUSI)
        {

        if( kp()->tilitpaatetty() >= kausi.paattyy() )
            return QIcon(":/pic/lukittu.png");
        }
        else if(  index.column() == ARKISTOITU)
        {
            if( kausi.arkistoitu() > kausi.viimeinenPaivitys() )
                return QIcon(":/pic/ok.png");
        }
        else if( index.column() == TILINPAATOS)
        {
            if( kausi.tilinpaatoksenTila() == Tilikausi::VAHVISTETTU)
                return QIcon(":/pic/ok.png");
            else if( kausi.tilinpaatoksenTila() == Tilikausi::KESKEN &&
                     kausi.paattyy().daysTo( kp()->paivamaara()) > 4 * 30)
                return QIcon(":/pic/varoitus.png");
            else if( kausi.paattyy().daysTo( kp()->paivamaara()) > 1 &&
                     kausi.paattyy().daysTo( kp()->paivamaara()) < 4 * 30 &&
                     kausi.tilinpaatoksenTila() != Tilikausi::EILAADITATILINAVAUKSELLE)
                return QIcon(":/pic/info.png");
        }
    }

    return QVariant();
}

void TilikausiModel::lisaaTilikausi(Tilikausi tilikausi)
{
    beginInsertRows( QModelIndex(), kaudet_.count(), kaudet_.count());

    kaudet_.append( tilikausi );
    kp()->tietokanta()->exec( QString("INSERT INTO tilikausi(alkaa,loppuu) VALUES('%1','%2') ")
                              .arg(tilikausi.alkaa().toString(Qt::ISODate))
                              .arg(tilikausi.paattyy().toString(Qt::ISODate)));
    endInsertRows();
}

void TilikausiModel::muokkaaViimeinenTilikausi(const QDate &paattyy)
{
    if( paattyy.isNull())
    {
        beginRemoveRows( QModelIndex(), kaudet_.count()-1, kaudet_.count()-1);
        kp()->tietokanta()->exec(QString("DELETE FROM tilikausi WHERE alkaa='%1' ").arg( kaudet_.last().alkaa().toString(Qt::ISODate) ) );
        kaudet_.removeLast();
        endRemoveRows();
    }
    else
    {
        kaudet_[ kaudet_.count()-1 ].paattyy() = paattyy;
        kp()->tietokanta()->exec(QString("UPDATE tilikausi SET loppuu='%1' WHERE alkaa='%2' ")
                          .arg( paattyy.toString(Qt::ISODate) ).arg( kaudet_.last().alkaa().toString(Qt::ISODate)) );
        emit dataChanged( index(kaudet_.count()-1, KAUSI), index(kaudet_.count()-1, KAUSI) );
    }
}

Tilikausi TilikausiModel::tilikausiPaivalle(const QDate &paiva) const
{
    foreach (Tilikausi kausi, kaudet_)
    {
        // Osuuko pyydetty päivä kysyttyyn jaksoon
        if( kausi.alkaa().daysTo(paiva) >= 0 && paiva.daysTo(kausi.paattyy()) >= 0)
            return kausi;
    }
    return Tilikausi(QDate(), QDate()); // Kelvoton tilikausi

}


int TilikausiModel::indeksiPaivalle(const QDate &paiva) const
{
    for(int i=0; i < kaudet_.count(); i++)
        if( kaudet_[i].alkaa().daysTo(paiva) >= 0 && paiva.daysTo(kaudet_[i].paattyy()) >= 0)
            return i;
    return -1;

}

Tilikausi TilikausiModel::tilikausiIndeksilla(int indeksi) const
{
    return kaudet_.value(indeksi, Tilikausi());
}

JsonKentta *TilikausiModel::json(int indeksi)
{
    return kaudet_[indeksi].json();
}

JsonKentta *TilikausiModel::json(Tilikausi tilikausi)
{
    return json( tilikausi.paattyy() );
}

JsonKentta *TilikausiModel::json(const QDate &paiva)
{
    return json( indeksiPaivalle( paiva) );
}


QDate TilikausiModel::kirjanpitoAlkaa() const
{
    if( kaudet_.count())
        return kaudet_.first().alkaa();
    return QDate();
}

QDate TilikausiModel::kirjanpitoLoppuu() const
{
    if( kaudet_.count())
        return kaudet_.last().paattyy();
    return QDate();
}

void TilikausiModel::lataa()
{
    beginResetModel();
    kaudet_.clear();

    QSqlQuery kysely(*tietokanta_);

    kysely.exec("SELECT alkaa, loppuu, json FROM tilikausi ORDER BY alkaa");
    while( kysely.next())
    {
        kaudet_.append( Tilikausi(kysely.value(0).toDate(), kysely.value(1).toDate(), kysely.value(2).toByteArray()));
    }
    endResetModel();
}

void TilikausiModel::tallenna()
{
    tietokanta_->transaction();

    QSqlQuery kysely(*tietokanta_);
    kysely.prepare("UPDATE tilikausi SET json=:json WHERE alkaa=:alku");
    for(int i=0; i<kaudet_.count(); i++)
    {
        if( kaudet_[i].json()->muokattu())
        {
            kysely.bindValue(":json", kaudet_[i].json()->toSqlJson());
            kysely.bindValue(":alku", kaudet_[i].alkaa());
            kysely.exec();
        }
    }

    tietokanta_->commit();
}
