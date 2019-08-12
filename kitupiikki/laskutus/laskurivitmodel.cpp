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
#include "laskurivitmodel.h"

#include "db/verotyyppimodel.h"
#include "db/kohdennus.h"
#include "db/kirjanpito.h"

LaskuRivitModel::LaskuRivitModel(QObject *parent)
    : QAbstractTableModel(parent)
{
}

QVariant LaskuRivitModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    // FIXME: Implement me!
    if( role == Qt::DisplayRole)
    {
        if( orientation == Qt::Horizontal)
        {
            switch (section)
            {
                case NIMIKE:
                    return tr("Nimike");
                case MAARA:
                    return tr("Määrä");
                case YKSIKKO:
                    return tr("Yksikkö");
                case AHINTA :
                    return tr("á netto");
                case ALE:
                    return  tr("Alennus");
                case ALV:
                    return tr("Alv");
                case TILI:
                    return tr("Tili");
                case KOHDENNUS:
                    return tr("Kohdennus");
                case BRUTTOSUMMA:
                    return tr("Yhteensä");
            }
        }
        else
            return QVariant( section + 1);
    }
    return QVariant();
}


int LaskuRivitModel::rowCount(const QModelIndex &parent) const
{
    if (parent.isValid())
        return 0;

    return rivit_.count();
}

int LaskuRivitModel::columnCount(const QModelIndex &parent) const
{
    if (parent.isValid())
        return 0;
    return 9;
}

QVariant LaskuRivitModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid())
        return QVariant();

    QVariantMap map = rivit_.at(index.row()).toMap();


    if( role == Qt::TextAlignmentRole)
    {
        if( index.column()==BRUTTOSUMMA || index.column() == MAARA || index.column() == ALV || index.column() == AHINTA || index.column() == ALE)
            return QVariant(Qt::AlignRight | Qt::AlignVCenter);
        else
            return QVariant( Qt::AlignLeft | Qt::AlignVCenter);

    } else if( role == Qt::DisplayRole) {
        switch (index.column()) {
        case NIMIKE:
            return map.value("nimike");
        case MAARA:
            return map.value("myyntikpl");
        case YKSIKKO:
            return map.value("yksikko");
        case AHINTA:
            return QString("%L1 €").arg(map.value("ahinta").toDouble(),0,'f',2);
        case ALE:
            if(map.contains("aleprosentti"))
                return QString("%1 %").arg(map.value("alvprosentti").toDouble());
            else
                return QString();
        case ALV:
            switch ( map.value("alvkoodi").toInt()) {
            case AlvKoodi::ALV0:
                return tr("0 %");
            case AlvKoodi::RAKENNUSPALVELU_MYYNTI:
                return tr("AVL 8c §");
            case AlvKoodi::YHTEISOMYYNTI_PALVELUT:
                return tr("AVL 65 §");
            case AlvKoodi::YHTEISOMYYNTI_TAVARAT:
                return tr("AVL 72a §");
            case AlvKoodi::MYYNNIT_MARGINAALI :
                return "Margin.";
            default:
                return QString("%1 %").arg( map.value("alvprosentti").toDouble() );
            }
        case KOHDENNUS:
            {
                Kohdennus kohdennus = kp()->kohdennukset()->kohdennus(  map.value("kohdennus").toInt());
                if( kohdennus.tyyppi() != Kohdennus::EIKOHDENNETA)
                    return kohdennus.nimi();
                else
                    return QVariant();
        }
        case TILI:
            return kp()->tilit()->tiliNumerolla( map.value("tili").toInt() ).nimiNumero();
        case BRUTTOSUMMA:
           return QString("%L1 €").arg(  riviSumma(map) ,0,'f',2);
        }
    }

    // FIXME: Implement me!
    return QVariant();
}

bool LaskuRivitModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
    if (data(index, role) != value) {
        // FIXME: Implement me!
        emit dataChanged(index, index, QVector<int>() << role);
        return true;
    }
    return false;
}

Qt::ItemFlags LaskuRivitModel::flags(const QModelIndex &index) const
{
    if (!index.isValid())
        return Qt::NoItemFlags;

    return Qt::ItemIsEditable; // FIXME: Implement me!
}

void LaskuRivitModel::lisaaTuote(QVariantMap tuote)
{
    if( !tuote.contains("myyntikpl"))
        tuote.insert("myyntikpl", 1.0);

    beginInsertRows( QModelIndex(), rivit_.count(), rivit_.count() );
    rivit_.append(tuote);
    endInsertRows();
}

double LaskuRivitModel::riviSumma(QVariantMap map)
{
    double maara = map.value("myyntikpl",1).toDouble();
    double ahinta = map.value("ahinta").toDouble();
    double alennus = map.value("aleprosentti",0).toDouble();
    double alvprossa = map.value("alvprosentti").toDouble();

    return map.value("alvkoodi").toInt() == AlvKoodi::MYYNNIT_MARGINAALI ?
                maara * ahinta * ( 100 - alennus) / 100 :
                maara * ahinta * ( 100 - alennus) * (100 + alvprossa) / 10000;

}
