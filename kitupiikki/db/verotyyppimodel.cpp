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

#include "verotyyppimodel.h"

VeroTyyppi::VeroTyyppi(AlvKoodi::Koodi uKoodi, const QString &uSelite, const QString &uKuvake)
    : koodi(uKoodi), selite(uSelite), kuvake( QIcon(uKuvake))
{

}


VerotyyppiModel::VerotyyppiModel(QObject *parent)
    : QAbstractListModel( parent)
{
    tyypit.append( VeroTyyppi(AlvKoodi::EIALV,"Veroton"));
    tyypit.append( VeroTyyppi(AlvKoodi::MYYNNIT_NETTO,"Verollinen myynti, nettokirjaus"));
    tyypit.append( VeroTyyppi(AlvKoodi::OSTOT_NETTO,"Verollinen osto, nettokirjaus"));
    tyypit.append( VeroTyyppi(AlvKoodi::MYYNNIT_BRUTTO,"Verollinen myynti, bruttokirjaus"));
    tyypit.append( VeroTyyppi(AlvKoodi::OSTOT_BRUTTO,"Verollinen osto, bruttokirjaus"));
    tyypit.append( VeroTyyppi(AlvKoodi::YHTEISOMYYNTI_TAVARAT,"Tavaroiden yhteisömyynti"));
    tyypit.append( VeroTyyppi(AlvKoodi::YHTEISOMYYNTI_PALVELUT,"Palveluiden yhteisömyynti"));
    tyypit.append( VeroTyyppi(AlvKoodi::YHTEISOHANKINNAT_TAVARAT,"Tavaroiden yhteisöhankinnat"));
    tyypit.append( VeroTyyppi(AlvKoodi::YHTEISOHANKINNAT_PALVELUT,"Palveluiden yhteisöhankinnat"));
    tyypit.append( VeroTyyppi(AlvKoodi::RAKENNUSPALVELU_MYYNTI,"Rakennuspalveluiden myynti"));
    tyypit.append( VeroTyyppi(AlvKoodi::RAKENNUSPALVELU_OSTO,"Rakennuspalveluiden osto"));
}

int VerotyyppiModel::rowCount(const QModelIndex & /*parent */) const
{
    return tyypit.count();
}

QVariant VerotyyppiModel::data(const QModelIndex &index, int role) const
{
    if( !index.isValid())
        return QVariant();

    VeroTyyppi tyyppi = tyypit.value(index.row());

    if( role == Qt::DisplayRole || role == SeliteRooli)
        return tyyppi.selite;
    else if( role == KoodiRooli)
        return tyyppi.koodi;
    else if( role == Qt::DecorationRole)
        return tyyppi.kuvake;

    return QVariant();
}

QString VerotyyppiModel::seliteKoodilla(int koodi) const
{
    foreach (VeroTyyppi tyyppi, tyypit)
    {
        if( tyyppi.koodi == koodi)
            return tyyppi.selite;
    }
    return QString();
}

