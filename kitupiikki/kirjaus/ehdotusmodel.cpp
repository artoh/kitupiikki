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

#include "ehdotusmodel.h"

EhdotusModel::EhdotusModel()
{

}

int EhdotusModel::rowCount(const QModelIndex & /* parent */) const
{
    return viennit_.count();
}

int EhdotusModel::columnCount(const QModelIndex & /* parent */) const
{
    return 3;
}

QVariant EhdotusModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if( role == Qt::TextAlignmentRole)
        return QVariant( Qt::AlignCenter | Qt::AlignVCenter);
    else if( role != Qt::DisplayRole )
        return QVariant();
    else if( orientation == Qt::Horizontal)
    {
        switch (section)
        {
            case TILI:
                return QVariant(tr("Tili"));
            case DEBET :
                return QVariant(tr("Debet"));
            case KREDIT:
                return QVariant(tr("Kredit"));
        }

    }
    return QVariant();
}

QVariant EhdotusModel::data(const QModelIndex &index, int role) const
{
    if( !index.isValid())
        return QVariant();

    VientiRivi rivi = viennit_.value( index.row() );

    if( role==Qt::DisplayRole || role == Qt::EditRole)
    {
        switch (index.column())
        {

            case TILI:
                if( rivi.tili.numero())
                    return QVariant( QString("%1 %2").arg(rivi.tili.numero()).arg(rivi.tili.nimi()) );
                else
                    return QVariant();

            case DEBET:
                if( role == Qt::EditRole)
                    return QVariant( rivi.debetSnt);
                else if( rivi.debetSnt )
                    return QVariant( QString("%L1 €").arg(rivi.debetSnt / 100.0,0,'f',2));
                else
                    return QVariant();

            case KREDIT:
                if( role == Qt::EditRole)
                    return QVariant( rivi.kreditSnt);
                else if( rivi.kreditSnt )
                    return QVariant( QString("%L1 €").arg(rivi.kreditSnt / 100.0,0,'f',2));
                else
                    return QVariant();
        }
    }
    else if( role == Qt::TextAlignmentRole)
    {
        if( index.column()==KREDIT || index.column() == DEBET)
            return QVariant(Qt::AlignRight | Qt::AlignVCenter);
        else
            return QVariant( Qt::AlignLeft | Qt::AlignVCenter);

    }

    return QVariant();
}

void EhdotusModel::tyhjaa()
{
    beginResetModel();
    viennit_.clear();
    endResetModel();
}

void EhdotusModel::lisaaVienti(VientiRivi rivi)
{
    beginInsertRows( QModelIndex(), viennit_.count(), viennit_.count());
    viennit_.append(rivi);
    endInsertRows();
}

void EhdotusModel::tallenna(VientiModel *model, int yhdistettavaVastatiliNumero, QDate yhdistettavaPvm)
{
    qlonglong yhdistettavaDebet = 0;
    qlonglong yhdistettavaKredit = 0;

    foreach (VientiRivi vienti, viennit_) {
        if( yhdistettavaVastatiliNumero &&
            yhdistettavaVastatiliNumero == vienti.tili.numero() &&
            yhdistettavaPvm == vienti.pvm )
        {
            yhdistettavaDebet += vienti.debetSnt;
            yhdistettavaKredit += vienti.kreditSnt;
        }
        else
        {
            model->lisaaVienti(vienti);
        }
    }

    // #44 Erien yhdistäminen
    if( yhdistettavaDebet || yhdistettavaKredit )
    {
        for( int i=0; i < model->rowCount(QModelIndex()); i++)
        {
            QModelIndex indeksi = model->index(i, 0);
            if( indeksi.data(VientiModel::TiliNumeroRooli).toInt() == yhdistettavaVastatiliNumero &&
                indeksi.data(VientiModel::PvmRooli).toDate() == yhdistettavaPvm )
            {
                // Lisätään tämä alkuperäiseen erään
                model->setData(indeksi, indeksi.data(VientiModel::DebetRooli).toLongLong() + yhdistettavaDebet , VientiModel::DebetRooli );
                model->setData(indeksi, indeksi.data(VientiModel::KreditRooli).toLongLong() + yhdistettavaKredit, VientiModel::KreditRooli );
                break;
            }
        }
    }

}

bool EhdotusModel::onkoKelpo(bool toispuolinen) const
{
    int debetSumma = 0;
    int kreditSumma = 0;

    foreach (VientiRivi rivi, viennit_)
    {
        debetSumma += rivi.debetSnt;
        kreditSumma += rivi.kreditSnt;
    }

    return ( debetSumma > 0 && ( debetSumma == kreditSumma || toispuolinen ));
}
