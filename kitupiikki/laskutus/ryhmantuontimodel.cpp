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
#include "ryhmantuontimodel.h"
#include "tuonti/csvtuonti.h"

#include <QFile>

RyhmanTuontiModel::RyhmanTuontiModel(QObject *parent)
    : QAbstractTableModel (parent)
{

}

int RyhmanTuontiModel::rowCount(const QModelIndex & /* parent */) const
{
    return csv_.count();
}

int RyhmanTuontiModel::columnCount(const QModelIndex & /* parent */) const
{
    if( csv_.isEmpty())
        return 0;
    return csv_.first().count();
}

QVariant RyhmanTuontiModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if( orientation == Qt::Horizontal && role == Qt::DisplayRole)
    {
        return otsikkoTeksti( sarakkeet_.at(section) );
    }

    return {};
}

QVariant RyhmanTuontiModel::data(const QModelIndex &index, int role) const
{
    if( !index.isValid())
        return {};

    if( role == Qt::DisplayRole)
    {
        return csv_.at(index.row()).at(index.column());
    }

    else if( role == Qt::TextColorRole)
    {
        if( index.row() == 0 && otsikkorivi_)
            return QColor(Qt::gray);
    }

    return {};
}

QString RyhmanTuontiModel::otsikkoTeksti(int sarakeEnum)
{
    switch (sarakeEnum) {
        case NIMI: return tr("Nimi");
        case LAHIOSOITE: return tr("Lähiosoite");
        case POSTIOSOITE: return tr("Postiosoite");
        case POSTINUMERO: return tr("Postinumero");
        case OSOITE: return tr("Osoite");
        case SAHKOPOSTI: return tr("Sähköposti");
        case YTUNNUS: return tr("Y-tunnus");
    default:
        return {};
    }
}

void RyhmanTuontiModel::lataaCsv(const QString &tiedostonNimi)
{
    QFile tiedosto(tiedostonNimi);
    if( !tiedosto.open(QIODevice::ReadOnly))
        return;

    beginResetModel();
    csv_ = CsvTuonti::csvListana( tiedosto.readAll() );

    if( !csv_.isEmpty())
        arvaaSarakkeet();

    endResetModel();
}

void RyhmanTuontiModel::asetaOtsikkoRivi(bool onko)
{
    otsikkorivi_ = onko;
    if( !csv_.isEmpty())
        emit dataChanged(index(0,0), index(0, csv_.first().size()));
}

void RyhmanTuontiModel::asetaMuoto(int sarake, int muoto)
{
    beginResetModel();
    sarakkeet_[sarake] = muoto;
    endResetModel();
}

void RyhmanTuontiModel::arvaaSarakkeet()
{
    // Ensiksi kokeillaan, josko ensimmäisellä rivillä olisi otsikkoja
    QVector<bool> loydetty(YTUNNUS+1);

    for(QString otsikko : csv_.first())
    {
        int tyyppi = EITUODA;
        for(int i=0; i<=YTUNNUS; i++)
        {
            if( !otsikko.compare( otsikkoTeksti(i), Qt::CaseInsensitive ) && !loydetty[i])
            {
                tyyppi = i;
                otsikkorivi_ = true;
                break;
            }
        }
        sarakkeet_.append(tyyppi);

    }
    // Sitten pitäisi varmaan vielä yrittää muodolla
}
