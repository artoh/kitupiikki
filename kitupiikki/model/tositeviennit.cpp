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
#include "tositeviennit.h"

#include <QDate>
#include "db/tili.h"
#include "db/verotyyppimodel.h"
#include "db/kirjanpito.h"
#include "db/tilinvalintadialogi.h"
#include "tosite.h"

#include <QDebug>

TositeViennit::TositeViennit(QObject *parent)
    : QAbstractTableModel(parent)
{
}

QVariant TositeViennit::headerData(int section, Qt::Orientation orientation, int role) const
{
    if( role == Qt::TextAlignmentRole)
        return QVariant( Qt::AlignCenter | Qt::AlignVCenter);
    else if( role != Qt::DisplayRole )
        return QVariant();
    else if( orientation == Qt::Horizontal)
    {
        switch (section)
        {
            case PVM:
                return tr("Pvm");
            case TILI:
                return tr("Tili");
            case DEBET :
                return tr("Debet");
            case KREDIT:
                return tr("Kredit");
            case KOHDENNUS :
                return tr("Kohdennus");
            case ALV:
                return tr("Alv");
            case SELITE:
                return tr("Selite");
        }
    }
    return QVariant( section + 1);
}


int TositeViennit::rowCount(const QModelIndex &parent) const
{
    if (parent.isValid())
        return 0;

    return viennit_.count();
}

int TositeViennit::columnCount(const QModelIndex &parent) const
{
    if (parent.isValid())
        return 0;

    return 7;
}

QVariant TositeViennit::data(const QModelIndex &index, int role) const
{
    if (!index.isValid())
        return QVariant();

    TositeVienti rivi = vienti(index.row());

    switch (role) {
    case Qt::DisplayRole :
        switch ( index.column()) {
        case PVM:
            return rivi.value("pvm").toDate();
        case TILI:
        {
            Tili *tili = kp()->tilit()->tiliPNumerolla( rivi.value("tili").toInt() );
            if( tili )
                return QString("%1 %2").arg(tili->numero()).arg(tili->nimi());
            return QVariant();
        }
        case DEBET:
        {
            double debet = rivi.value("debet").toDouble();
             if( debet > 1e-5 )
                return QVariant( QString("%L1 €").arg(debet,0,'f',2));
             return QVariant();
        }
        case KREDIT:
        {
            double kredit = rivi.value("kredit").toDouble();
            if( kredit > 1e-5)
                return QVariant( QString("%L1 €").arg(kredit,0,'f',2));
             return QVariant();
        }
        case ALV:
        {
            int alvkoodi = rivi.value("alvkoodi").toInt();
            if( alvkoodi == AlvKoodi::EIALV )
                return QVariant();
            else
            {
                if( alvkoodi == AlvKoodi::MAKSETTAVAALV)
                    return tr("VERO");
                else if(alvkoodi == AlvKoodi::TILITYS)
                    return QString();
                else
                    return QVariant( QString("%1 %").arg( rivi.value("alvprosentti").toInt() ));
            }
        }
        case SELITE:
            return rivi.value("selite");
        case KOHDENNUS:
            QString txt;
            int kohdennus = rivi.value("kohdennus").toInt();
            if( kohdennus )
                txt.append( kp()->kohdennukset()->kohdennus(kohdennus).nimi() + " " );
            QVariantList merkkaukset = rivi.value("merkkaukset").toList();
            for( auto merkkaus : merkkaukset)
                txt.append( kp()->kohdennukset()->kohdennus( merkkaus.toInt() ).nimi() + " " );

            if( rivi.value("era").toMap().contains("tunniste")  )
            {
                if( rivi.value("era").toMap().value("tunniste") != rivi.value("tosite").toMap().value("tunniste") ||
                    rivi.value("era").toMap().value("pvm") != rivi.value("tosite").toMap().value("pvm")) {
                    if( !txt.isEmpty())
                        txt.append(" ");
                    txt.append( QString("%1/%2")
                            .arg( rivi.value("era").toMap().value("tunniste").toInt() )
                            .arg( kp()->tilikaudet()->tilikausiPaivalle( rivi.value("era").toMap().value("pvm").toDate() ).kausitunnus()) );
                }
            }
            return txt;

        }
        break;
    case Qt::EditRole :
        switch ( index.column())
        {
        case PVM:
            return rivi.value("pvm").toDate();
        case TILI:
            return rivi.value("tili").toInt();
        case DEBET:
            return rivi.value("debet").toDouble();
        case KREDIT:
            return rivi.value("kredit").toDouble();
        }
        break;
    case Qt::TextAlignmentRole:
        if( index.column()==KREDIT || index.column() == DEBET || index.column() == ALV)
            return QVariant(Qt::AlignRight | Qt::AlignVCenter);
        else
            return QVariant( Qt::AlignLeft | Qt::AlignVCenter);

    case Qt::DecorationRole:
        if( index.column() == ALV )
        {
            return kp()->alvTyypit()->kuvakeKoodilla( rivi.data(TositeVienti::ALVKOODI).toInt() );
        } else if( index.column() == KOHDENNUS ) {
            if( rivi.contains("era") && rivi.value("era").toMap().value("saldo") == 0 )
                return QIcon(":/pic/ok.png");
            Kohdennus kohdennus = kp()->kohdennukset()->kohdennus( rivi.value("kohdennus").toInt() );
            if(kohdennus.tyyppi())
                return kp()->kohdennukset()->kohdennus( rivi.value("kohdennus").toInt()).tyyppiKuvake();
            else
                return QIcon(":/pic/tyhja.png");
        } else if( index.column() == PVM)
        {
            // Väärät päivät
            QDate pvm = rivi.pvm();
            if( pvm.isValid() )
                return QVariant();
            else if( rivi.id() && pvm <= kp()->tilitpaatetty())
                return QIcon(":/pic/lukittu.png");
            else if( pvm <= kp()->tilitpaatetty() || pvm > kp()->tilikaudet()->kirjanpitoLoppuu() )
                return QIcon(":/pic/varoitus.png");
            else if( kp()->asetukset()->pvm("AlvIlmoitus") >= pvm && rivi.alvkoodi() )
                return QIcon(":/pic/vero.png");
        }
        break;
    case IdRooli:
        return rivi.id();
    case PvmRooli:
        return rivi.pvm();
    case TiliNumeroRooli:
        return rivi.tili();
    case DebetRooli:
        return rivi.debet();
    case KreditRooli:
        return rivi.kredit();
    case AlvKoodiRooli:
        return rivi.alvKoodi();
    case AlvProsenttiRooli:
        return rivi.alvProsentti();
    case KohdennusRooli:
        return rivi.kohdennus();
    case SeliteRooli:
        return rivi.selite();
    case EraIdRooli:
        return rivi.eraId();
    case TaseErittelyssaRooli:
    {
        Tili tili = kp()->tilit()->tiliNumerolla( rivi.tili() );
        return tili.eritellaankoTase();
    }
    case TagiIdListaRooli:
        return rivi.data(TositeVienti::MERKKAUKSET);


    }

    return QVariant();
}

bool TositeViennit::setData(const QModelIndex &index, const QVariant &value, int role)
{
    if (data(index, role) != value) {
        if( role == Qt::EditRole) {

            TositeVienti rivi = vienti(index.row());

            switch (index.column()) {



            case PVM:
                rivi.setPvm( value.toDate() );
                break;
            case TILI:
                {
                    Tili uusitili;
                    if( value.toInt())
                        uusitili = kp()->tilit()->tiliNumerolla( value.toInt());
                    else if(!value.toString().isEmpty() && value.toString() != " ")
                        uusitili = TilinValintaDialogi::valitseTili(value.toString());
                    else
                        uusitili = TilinValintaDialogi::valitseTili( QString());

                    rivi.setTili( uusitili.numero());
                    if( uusitili.eritellaankoTase())
                        rivi.setEra( TaseEra::UUSIERA);
                    else
                        rivi.setEra( TaseEra::EIERAA);
                    break;
                }
            case SELITE:
                rivi.setSelite( value.toString());
                break;
            case DEBET:
                rivi.setDebet( value.toDouble() );
                break;
            case KREDIT:
                rivi.setKredit( value.toDouble() );
                break;
            case KOHDENNUS:
                rivi.setKohdennus( value.toInt() );
                break;
            default:
                return false;

            }

            viennit_[index.row()] = rivi;
        }
        emit dataChanged(index, index, QVector<int>() << role);
        return true;
    }
    return false;
}

Qt::ItemFlags TositeViennit::flags(const QModelIndex &index) const
{
    if (!index.isValid())
        return Qt::NoItemFlags;

    if( muokattavissa_ )
        return Qt::ItemIsEditable | Qt::ItemIsEnabled; // FIXME: Implement me!

    return Qt::ItemIsEnabled;
}

bool TositeViennit::insertRows(int row, int count, const QModelIndex & /* parent */)
{
    for(int i=0; i < count; i++)
        lisaaVienti(row);
    return true;
}


bool TositeViennit::removeRows(int row, int count, const QModelIndex &parent)
{
    beginRemoveRows(parent, row, row + count - 1);
    for(int i=0; i < count; i++)
        viennit_.removeAt(row);
    endRemoveRows();
    return true;
}

QModelIndex TositeViennit::lisaaVienti(int indeksi)
{
    TositeVienti uusi;

    Tosite* tosite = qobject_cast<Tosite*>(parent());
    uusi.setPvm( tosite->data(Tosite::PVM).toDate() );

    beginInsertRows( QModelIndex(), indeksi, indeksi);
    viennit_.insert(indeksi, uusi);
    endInsertRows();
    return index(indeksi, 0);
}

TositeVienti TositeViennit::vienti(int indeksi) const
{
    return TositeVienti( viennit_.at(indeksi).toMap() );
}

void TositeViennit::lisaa(const TositeVienti &vienti)
{
    beginInsertRows(QModelIndex(), viennit_.count(), viennit_.count()+1);
    viennit_.append(vienti);
    endInsertRows();
}


void TositeViennit::asetaViennit(QVariantList viennit)
{
    beginResetModel();
    viennit_ = viennit;
    // Erätietojen siivoaminen ja sijoittaminen välimuistiin
    endResetModel();

    qDebug() << viennit_;
}

void TositeViennit::asetaMuokattavissa(bool muokattavissa)
{
    muokattavissa_ = muokattavissa;
}
