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
#include "yksikkomodel.h"

YksikkoModel::YksikkoModel(QObject *parent)
    : QAbstractListModel(parent)
{
    // Muut yksiköt saavat UN-koodin ZZ Mutually defined
    lisaa("C62",tr("Kappale"));
    lisaa("NMP", tr("Pakkaus"));
    lisaa("IE", tr("Henkilö"));
    lisaa("KGM", tr("Kilogramma"));
    lisaa("LTR", tr("Litra"));
    lisaa("ANN", tr("Vuosi"));
    lisaa("MON", tr("Kuukausi"));
    lisaa("DAY", tr("Päivä"));
    lisaa("HUR", tr("Tunti"));
    lisaa("MIN", tr("Minuutti"));
    lisaa("LH", tr("Tunti työtä"));
    lisaa("ACT", tr("Yksikkö työtä"));
    lisaa("E50", tr("Laskutusyksikkö"));
    lisaa("E51", tr("Työsuorite"));
    lisaa("KMT", tr("Kilometri"));
    lisaa("KWH", tr("Kilowattitunti"));
    lisaa("MTK", tr("Neliömetri"));
    lisaa("MTQ", tr("Kuutiometri"));
    lisaa("ZP", tr("Sivu"));
}

int YksikkoModel::rowCount(const QModelIndex &parent) const
{
    // For list models only the root node (an invalid parent) should return the list's size. For all
    // other (valid) parents, rowCount() should return 0 so that it does not become a tree model.
    if (parent.isValid())
        return 0;

    return yksikot_.count();
}

QVariant YksikkoModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid())
        return QVariant();

    const Yksikko& yksikko = yksikot_.at(index.row());

    switch (role) {
    case Qt::DisplayRole:
        return yksikko.nimi();
    case UNKoodiRooli:
        return yksikko.unKoodi();
    default:
        return QVariant();
    }
}

QString YksikkoModel::nimi(const QString &unKoodi) const
{
    for( const auto& yksikko: yksikot_) {
        if( yksikko.unKoodi() == unKoodi) {
            return yksikko.nimi();
        }
    }
    return QString();
}

void YksikkoModel::lisaa(const QString &UNkoodi, const QString &nimi)
{
    yksikot_.append(Yksikko(UNkoodi, nimi));
}

YksikkoModel::Yksikko::Yksikko()
{

}

YksikkoModel::Yksikko::Yksikko(const QString &UNkoodi, const QString &nimi)
    : UNkoodi_(UNkoodi), nimi_(nimi)
{

}
