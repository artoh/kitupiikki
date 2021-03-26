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
#include "maamodel.h"
#include <QIcon>
#include <QDebug>

MaaModel::MaaModel(QObject *parent)
    : QAbstractListModel(parent)
{
    lisaa("fi","Suomi","FI\\d{8}");
    lisaa("be","Belgia","BE\\d{10}");
    lisaa("bg","Bulgaria","BG\\d{9,10}");
    lisaa("es","Espanja","ES\\w\\d{7}\\w");
    lisaa("nl","Hollanti","NL\\d{9}B\\d{2}");
    lisaa("ie","Irlanti","IE\\w{8,9}");
    lisaa("gb","Iso-Britannia","GB\\w{5,12}");
    lisaa("ie","Italia","IT\\d{11}");
    lisaa("at","Itävalta","ATU\\d{8}");
    lisaa("el","Kreikka","EL\\d{9}");
    lisaa("hr","Kroatia","HR\\d{11}");
    lisaa("cy","Kypros","CY\\d{8}\\w");
    lisaa("lv","Latvia","LV\\d{11}");
    lisaa("lt","Liettua","LT\\d{9,12}");
    lisaa("lu","Luxemburg","LU\\d{8}");
    lisaa("mt","Malta","MT\\d{8}");
    lisaa("pt","Portugali","PT\\d{9}");
    lisaa("pl","Puola","PL\\d{10}");
    lisaa("fr","Ranska","FR\\w{2}\\s?\\d{9}");
    lisaa("ro","Romania","RO\\d{2,10}");
    lisaa("se","Ruotsi","SE\\d{9}01");
    lisaa("de","Saksa","DE\\d{9}");
    lisaa("sk","Slovakia","SL\\d{10}");
    lisaa("si","Slovenia","SI\\d{8}");
    lisaa("dk","Tanska","DK\\d{2}\\s?\\d{2}\\s?\\d{2}\\s?\\d{2}");
    lisaa("cz","Tsekki","CZ\\d{8,10}");
    lisaa("hu","Unkari","HU\\d{8}");
    lisaa("ee","Viro","EE\\d{9}");
    lisaa("","EU:n ulkopuolella","");

}


int MaaModel::rowCount(const QModelIndex &parent) const
{
    // For list models only the root node (an invalid parent) should return the list's size. For all
    // other (valid) parents, rowCount() should return 0 so that it does not become a tree model.
    if (parent.isValid())
        return 0;

    // FIXME: Implement me!
    return maat_.count();

}

QVariant MaaModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid())
        return QVariant();

    if( role == Qt::DisplayRole)
        return maat_.at(index.row()).nimi();
    else if( role == Qt::DecorationRole)
        return maat_.at(index.row()).icon();
    else if( role == KoodiRooli)
        return maat_.at(index.row()).koodi();
    else if( role == AlvRegExpRooli)
        return maat_.at(index.row()).alvreg();

    // FIXME: Implement me!
    return QVariant();
}

MaaModel *MaaModel::instanssi()
{
    if( !instanssi__)
        instanssi__ = new MaaModel();
    return instanssi__;
}

QString MaaModel::muotoiltuOsoite(const QVariantMap &kumppani) const
{
    const QString& nimi = kumppani.value("nimi").toString();
    const QString& osoite = kumppani.value("osoite").toString();
    const QString postinumero = kumppani.value("postinumero").toString();
    const QString& kaupunki = kumppani.value("kaupunki").toString();
    const QString& maa = kumppani.value("maa").toString();

    if( maa != "fi")
        qWarning() << "Osoite maahan " << maa;

    // TODO: Ulkomaiset osoitteet
    return nimi + "\n" +
           osoite + "\n" +
           postinumero + " " + kaupunki;
}

void MaaModel::lisaa(const QString &koodi, const QString &nimi, const QString &regexp)
{
    maat_.append( Maa(koodi, nimi, regexp) );
}

MaaModel::Maa::Maa(const QString &koodi, const QString &nimi, const QString &alvreg) :
    nimi_(nimi),
    koodi_(koodi),
    alvreg_(alvreg),
    icon_( QIcon(":/liput/" + koodi + ".png"))
{

}

MaaModel* MaaModel::instanssi__ = nullptr;
