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
    return 3;
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
        case SENTIT :
            return QVariant("Budjetti");
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

            case SENTIT:
                QString tilinumero = proxy_->data( proxy_->index(index.row(), TiliModel::NUMERO) ).toString();
                qlonglong sentit = sentit_.value( tilinumero, "0").toLongLong() ;
                if( role == Qt::EditRole)
                    return QVariant(sentit / 100.0);

                if( sentit )
                    return QVariant( QString("%L1 €").arg( sentit / 100.0, 10,'f',2));
                else
                    return QVariant();

        }
    }
    else if( role == Qt::TextAlignmentRole)
    {
        if( index.column()==SENTIT)
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


    return QVariant();


}

Qt::ItemFlags BudjettiModel::flags(const QModelIndex &index) const
{

    if( index.column() == SENTIT && !proxy_->data( proxy_->index(index.row(),0), TiliModel::OtsikkotasoRooli ).toInt()  )
        return QAbstractTableModel::flags(index) | Qt::ItemIsEditable;
    else
        return QAbstractTableModel::flags(index);
}

bool BudjettiModel::setData(const QModelIndex &index, const QVariant &value, int /* role */)
{
    QString tilinumero = proxy_->data( proxy_->index(index.row(), TiliModel::NUMERO) ).toString();

    if( value.toInt())
        sentit_[tilinumero] = qRound(value.toDouble() * 100); // Delegaatti käsittelee senttejä
    else
        sentit_.remove(tilinumero);          // Ei jätetä nollia kirjauksiin

    muokattu_ = true;
    laskeSumma();
    return true;
}

void BudjettiModel::lataa(const QDate &paivamaara, int kohdennusid)
{
    beginResetModel();
    paivamaara_ = paivamaara;
    kohdennusid_ = kohdennusid;

    QVariantMap varmap = kp()->tilikaudet()->json(paivamaara_)->variant("Budjetti").toMap();
    sentit_ = varmap.value( QString::number(kohdennusid_) ).toMap();
    laskeSumma();

    endResetModel();
}

void BudjettiModel::tallenna()
{
    JsonKentta *json = kp()->tilikaudet()->json(paivamaara_);

    QVariantMap varmap = json->variant("Budjetti").toMap();

    if( sentit_.isEmpty())
        varmap.remove(QString::number(kohdennusid_) );
    else
        varmap.insert( QString::number(kohdennusid_), sentit_ );

    json->setVar("Budjetti", varmap);
    kp()->tilikaudet()->tallennaJSON();

    muokattu_ = false;
    laskeSumma();
}

void BudjettiModel::laskeSumma()
{
    qlonglong summa = 0;
    for( QVariant var : sentit_.values() )
        summa += var.toLongLong();

    emit summaMuuttui(summa);

}

void BudjettiModel::kopioiEdellinen()
{
    QDate pvm = kp()->tilikaudet()->tilikausiPaivalle( paivamaara_.addDays(-1) ).alkaa();

    beginResetModel();
    QVariantMap varmap = kp()->tilikaudet()->json(pvm)->variant("Budjetti").toMap();
    sentit_ = varmap.value( QString::number(kohdennusid_) ).toMap();
    muokattu_ = true;
    laskeSumma();

    endResetModel();
}


