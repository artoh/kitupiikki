/*
   Copyright (C) 2018 Arto Hyvättinen

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
#include "budjettimodel.h"
#include "db/kirjanpito.h"

#include <QSortFilterProxyModel>
#include <QPalette>
#include <QFont>

BudjettiModel::BudjettiModel(QObject *parent)
    : QAbstractTableModel(parent)
{
    QSortFilterProxyModel *tilaproxy = new QSortFilterProxyModel(this);
    tilaproxy->setSourceModel( kp()->tilit());
    tilaproxy->setFilterRole(TiliModel::TilaRooli);
    tilaproxy->setFilterRegExp(QRegExp("[12]"));

    proxy_ = new QSortFilterProxyModel(this);
    proxy_->setSourceModel( tilaproxy );
    proxy_->setFilterKeyColumn( TiliModel::NUMERO );
    proxy_->setFilterRegExp( QRegExp("^[3-9].*"));
}

int BudjettiModel::rowCount(const QModelIndex &parent) const
{
    return proxy_->rowCount(parent);
}

int BudjettiModel::columnCount(const QModelIndex & /* parent */) const
{
    return 4;
}

QVariant BudjettiModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if( role == Qt::TextAlignmentRole)
        return QVariant( Qt::AlignCenter | Qt::AlignVCenter);
    else if( orientation == Qt::Horizontal && role == Qt::DisplayRole )
    {
        switch (section)
        {
        case NRO :
            return QVariant("Numero");
        case NIMI:
            return QVariant("Tili");
        case EDELLINEN:
            return QVariant("Ed. budjetti €");
        case EUROT :
            return QVariant("Uusi budjetti €");
        }
    }
    return QVariant();
}

QVariant BudjettiModel::data(const QModelIndex &index, int role) const
{
    if( !index.isValid())
        return QVariant();

    if( role == Qt::DisplayRole || role == Qt::EditRole)
    {
        switch (index.column())
        {
            case NRO:
                return proxy_->data( proxy_->index(index.row(), TiliModel::NUMERO) ) ;

            case NIMI:
                return proxy_->data( proxy_->index(index.row(), TiliModel::NIMI) ) ;

            case EUROT:
                if( proxy_->data( proxy_->index(index.row(), 0), TiliModel::OtsikkotasoRooli ).toInt() )
                    return QVariant();

                QString tilinumero = proxy_->data( proxy_->index(index.row(), TiliModel::NUMERO) ).toString();

                double eurot = data_.value( QString::number(kohdennusid_) ).toMap().value( tilinumero ).toDouble();
                if( role == Qt::EditRole)
                    return eurot;

                if( qAbs(eurot) > 1e-7 )
                    return QVariant( QString("%L1 €").arg( eurot, 10,'f',2));
                else
                    return QVariant();

        }
    }
    else if( role == Qt::TextAlignmentRole)
    {
        if( index.column()==EUROT)
            return QVariant(Qt::AlignRight | Qt::AlignVCenter);
        else
            return QVariant( Qt::AlignLeft | Qt::AlignVCenter);

    }
    else if( role == Qt::FontRole)
    {
        QFont fontti;
        if( proxy_->data( proxy_->index(index.row(), TiliModel::NUMERO), TiliModel::OtsikkotasoRooli ).toInt() )
            fontti.setBold(true);
        return QVariant( fontti );
    }

    else if( role==Qt::BackgroundColorRole)
    {
        if( proxy_->data( proxy_->index(index.row(), TiliModel::NUMERO), TiliModel::OtsikkotasoRooli ).toInt() )
            return QPalette().mid().color();
    }


    return QVariant();


}

Qt::ItemFlags BudjettiModel::flags(const QModelIndex &index) const
{

    if( index.column() == EUROT && !proxy_->data( proxy_->index(index.row(),0), TiliModel::OtsikkotasoRooli ).toInt()  )
        return QAbstractTableModel::flags(index) | Qt::ItemIsEditable;
    else
        return QAbstractTableModel::flags(index);
}

bool BudjettiModel::setData(const QModelIndex &index, const QVariant &value, int /* role */)
{
    QString tilinumero = proxy_->data( proxy_->index(index.row(), TiliModel::NUMERO) ).toString();

    QVariantMap eurot = data_.value( QString::number(kohdennusid_) ).toMap();

    if( qAbs( value.toDouble()) > 1e-5)
        eurot[tilinumero] = value.toDouble();
    else
        eurot.remove(tilinumero);          // Ei jätetä nollia kirjauksiin

    data_.insert( QString::number(kohdennusid_), eurot );

    muokattu_ = true;
    laskeSumma();
    return true;
}

void BudjettiModel::lataa(const QDate &paivamaara)
{
    paivamaara_ = paivamaara;

    KpKysely *kysely = kpk(QString("/budjetti/%1").arg(paivamaara.toString(Qt::ISODate)));
    kysely->lisaaAttribuutti("kohdennukset");
    connect( kysely, &KpKysely::vastaus, this, &BudjettiModel::dataSaapuu);
    kysely->kysy();

    QDate edellisenPvm = kp()->tilikaudet()->tilikausiPaivalle( paivamaara_.addDays(-1) ).alkaa();
    KpKysely *edkysely = kpk(QString("/budjetti/%1").arg(edellisenPvm.toString(Qt::ISODate)));
    edkysely->lisaaAttribuutti("kohdennukset");
    connect( edkysely, &KpKysely::vastaus, this, &BudjettiModel::edellinenSaapuu);
    edkysely->kysy();
}

void BudjettiModel::nayta(int kohdennus)
{
    kohdennusid_ = kohdennus;
    emit dataChanged( index(0, EDELLINEN), index( rowCount(), EUROT ) );
    laskeSumma();
}

void BudjettiModel::tallenna()
{

    KpKysely *kysely = kpk(QString("/budjetti/%1").arg(paivamaara_.toString(Qt::ISODate)), KpKysely::PUT);
    kysely->connect( kysely, &KpKysely::vastaus, this, &BudjettiModel::tallennettu);
    kysely->kysy( data_ );

}

void BudjettiModel::laskeSumma()
{
    qlonglong summa = 0;
    QVariantMap eurot = data_.value( QString::number(kohdennusid_) ).toMap();

    for( const QVariant& var : eurot.values() )
        summa += qRound( var.toDouble() * 100.0 );

    qlonglong kokosumma = 0;

    QMapIterator<QString,QVariant> iter(data_);
    while( iter.hasNext()) {
        iter.next();
        QVariantMap map = iter.value().toMap();
        for( auto var : map.values())
            kokosumma += qRound( var.toDouble() * 100.0);
    }

    emit summaMuuttui(summa, kokosumma);

}

void BudjettiModel::kopioiEdellinen()
{
    data_ = edellinen_;
    emit dataChanged( index(0, EUROT), index( rowCount(), EUROT ) );
}

void BudjettiModel::dataSaapuu(QVariant *saapuva)
{
    data_ = saapuva->toMap();
    emit dataChanged( index(0, EUROT), index( rowCount(), EUROT ) );
    laskeSumma();
}

void BudjettiModel::edellinenSaapuu(QVariant *saapuva)
{
    edellinen_ = saapuva->toMap();
    emit dataChanged( index(0, EDELLINEN), index( rowCount(), EDELLINEN ) );
}

void BudjettiModel::tallennettu()
{
    this->muokattu_=false;
    this->laskeSumma();
}


