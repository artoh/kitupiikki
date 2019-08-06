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
#include "tiliotemodel.h"
#include "db/kirjanpito.h"
#include "db/tilinvalintadialogi.h"

#include "model/tositevienti.h"

TilioteModel::TilioteModel(QObject *parent)
    : QAbstractTableModel(parent)
{
}

QVariant TilioteModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    // FIXME: Implement me!
    if( role == Qt::DisplayRole && orientation == Qt::Horizontal)
    {
        switch (section)
        {
        case PVM:
            return tr("Pvm");
        case EURO:
            return tr("Euro");
        case TILI:
            return tr("Tili");
        case KOHDENNUS:
            return tr("Kohdennus");
        case SAAJAMAKSAJA:
            return tr("Saaja/Maksaja");
        case SELITE:
            return tr("Selite");

        }
    }
    return QVariant();
}

bool TilioteModel::setHeaderData(int section, Qt::Orientation orientation, const QVariant &value, int role)
{
    if (value != headerData(section, orientation, role)) {
        // FIXME: Implement me!
        emit headerDataChanged(orientation, section, section);
        return true;
    }
    return false;
}


int TilioteModel::rowCount(const QModelIndex &parent) const
{
    if (parent.isValid())
        return 0;

    return rivit_.count();
}

int TilioteModel::columnCount(const QModelIndex &parent) const
{
    if (parent.isValid())
        return 0;

    return 6;
}

QVariant TilioteModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid())
        return QVariant();

    Tilioterivi rivi = rivit_.at(index.row());


    switch ( role ) {
    case Qt::DisplayRole :
        switch (index.column()) {
        case PVM:
            return rivi.pvm;
        case EURO:
            if( qAbs(rivi.euro) > 1e-5)
                return QString("%L1 €").arg( rivi.euro ,0,'f',2);
            return QString();
        case TILI:
            return kp()->tilit()->tiliNumerolla( rivi.tili ).nimiNumero();
        case KOHDENNUS:
        {
            if( rivi.eraId) {
                if( rivi.laskupvm.isValid())
                    return rivi.laskupvm;
                return rivi.eraTunnus;
            }
            QString txt;
            if( rivi.kohdennus )
                txt = kp()->kohdennukset()->kohdennus(rivi.kohdennus).nimi() + " ";
            QStringList merkkausList;
            for(auto mid : rivi.merkkaukset)
                merkkausList.append( kp()->kohdennukset()->kohdennus(mid).nimi() );
            txt.append( merkkausList.join(", ") );
            return txt;
        }
        case SAAJAMAKSAJA:
            return rivi.saajamaksaja;
        case SELITE:
            return  rivi.selite;
        }

    case Qt::EditRole :
        switch ( index.column())
        {
        case PVM:
            return rivi.pvm;
        case TILI:
            return rivi.tili;
        case EURO:
            return qRound( rivi.euro * 100 );
        case KOHDENNUS:
            return rivi.kohdennus;
        case SAAJAMAKSAJA:
            return rivi.saajamaksajaId;
        case SELITE:
            return rivi.selite;
        }

    case Qt::TextAlignmentRole:
        if( index.column()==EURO)
            return QVariant(Qt::AlignRight | Qt::AlignVCenter);
        else
            return QVariant( Qt::AlignLeft | Qt::AlignVCenter);

    case Qt::DecorationRole:
        if( index.column() == KOHDENNUS) {
            if( rivi.laskupvm.isValid())
                return QIcon(":/pic/lasku.png");
        }

    }
    return QVariant();
}

bool TilioteModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
    if (data(index, role) != value) {

        if( role == Qt::EditRole) {
            switch (index.column()) {
                case PVM :
                    rivit_[index.row()].pvm = value.toDate();
                break;
            case TILI: {
                Tili uusitili;
                if( value.toInt())
                    uusitili = kp()->tilit()->tiliNumerolla( value.toInt());
                else if(!value.toString().isEmpty() && value.toString() != " ")
                    uusitili = TilinValintaDialogi::valitseTili(value.toString());
                else
                    uusitili = TilinValintaDialogi::valitseTili( QString());
                rivit_[ index.row()].tili = uusitili.numero();
                break;
                }
            case EURO:
                rivit_[ index.row()].euro = value.toDouble() ;
                break;
            case KOHDENNUS:
                rivit_[ index.row()].kohdennus = value.toInt();
                break;
            case SELITE:
                rivit_[index.row()].selite = value.toString();
            }
        }

        emit dataChanged(index, index, QVector<int>() << role);
        return true;
    }
    return false;
}

Qt::ItemFlags TilioteModel::flags(const QModelIndex &index) const
{
    if (!index.isValid())
        return Qt::NoItemFlags;

    if( index.column() == KOHDENNUS )
    {
        Tili tili = kp()->tilit()->tiliNumerolla( rivit_.at(index.row()).tili );
        if( !tili.onko(TiliLaji::TULOS))
            return Qt::ItemIsEnabled;
    }

    return Qt::ItemIsEditable | Qt::ItemIsEnabled; // FIXME: Implement me!
}

void TilioteModel::lisaaRivi(const TilioteModel::Tilioterivi &rivi)
{
    beginInsertRows( QModelIndex(), rowCount(), rowCount());
    rivit_.append(rivi);
    endInsertRows();
}

void TilioteModel::poistaRivi(int rivi)
{
    beginRemoveRows(QModelIndex(), rivi, rivi);
    rivit_.removeAt(rivi);
    endRemoveRows();
}

void TilioteModel::muokkaaRivi(int rivi, const TilioteModel::Tilioterivi &data)
{
    rivit_[rivi] = data;
    emit dataChanged( index(rivi,PVM), index(rivi,SELITE) );
}

QVariantList TilioteModel::viennit(int tilinumero) const
{
    QVariantList lista;

    for(auto rivi : rivit_) {
        if( qAbs( rivi.euro ) > 1e-5 && rivi.tili ) {
            TositeVienti pankki;
            TositeVienti tili;

            pankki.setPvm( rivi.pvm );
            tili.setPvm( rivi.pvm );

            pankki.setTili(tilinumero);
            tili.setTili( rivi.tili );

            if( rivi.euro > 0.0) {
                pankki.setDebet( rivi.euro);
                tili.setKredit( rivi.euro);
            } else {
                pankki.setKredit( 0.0 - rivi.euro);
                tili.setDebet( 0.0 - rivi.euro);
            }

            tili.setKohdennus( rivi.kohdennus);

            if( rivi.eraId)
                tili.setEra( rivi.eraId );

            pankki.setSelite( rivi.selite );
            tili.setSelite( rivi.selite );

            // TODO: Arkistotunnus, tilinumero, viite yms. metatieto
            pankki.setArkistotunnus( rivi.arkistotunnus );

            if( rivi.saajamaksajaId)
                {
                if( rivi.euro > 0.0) {
                    pankki.set(TositeVienti::ASIAKAS, rivi.saajamaksajaId);
                    tili.set(TositeVienti::ASIAKAS, rivi.saajamaksajaId);
                } else {
                    pankki.set(TositeVienti::TOIMITTAJA, rivi.saajamaksajaId);
                    tili.set(TositeVienti::TOIMITTAJA, rivi.saajamaksajaId);
                }
            }

            lista.append(pankki);
            lista.append(tili);
        }
    }
    return lista;
}

void TilioteModel::lataa(QVariantList lista)
{
    beginResetModel();
    rivit_.clear();
    // Haetaan ainoastaan joka toinen rivi eli vientirivit, kaikki muuthan koskeekin pankkitiliä
    for(int i=1; i < lista.count(); i+=2)
    {
        TositeVienti vienti = lista.at(i).toMap();
        TositeVienti pankki = lista.at(i-1).toMap();
        Tilioterivi rivi;

        bool meno = pankki.kredit() > 0;

        rivi.pvm = vienti.pvm();
        rivi.euro = meno ? pankki.kredit() : pankki.debet();
        rivi.tili = vienti.tili();
        rivi.kohdennus = vienti.kohdennus();
        rivi.merkkaukset = vienti.merkkaukset();
        rivi.eraId = vienti.eraId();

        if( vienti.eraId() ) {
            rivi.laskupvm = vienti.value("era").toMap().value("pvm").toDate();
        }

        if( meno && pankki.contains("toimittaja")) {
            rivi.saajamaksajaId = pankki.value("toimittaja").toMap().value("id").toInt();
            rivi.saajamaksaja = pankki.value("toimittaja").toMap().value("nimi").toString();
        } else if( !meno && pankki.contains("asiakas")) {
            rivi.saajamaksajaId = pankki.value("asiakas").toMap().value("id").toInt();
            rivi.saajamaksaja = pankki.value("asiakas").toMap().value("nimi").toString();
        }

        rivi.selite = vienti.selite();
        rivi.arkistotunnus = pankki.arkistotunnus();

        rivit_.append(rivi);
    }
    endResetModel();
}

