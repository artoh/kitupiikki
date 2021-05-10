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

#include <QFile>
#include <QTextStream>


MaaModel::MaaModel(QObject *parent)
    : QAbstractListModel(parent)
{
    lataa();
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
    const QString& osavaltio = kumppani.value("osavaltio").toString();
    const QString& maa = kumppani.value("maa").toString();
    const Maa& maaData = maaKoodilla(maa);
    const QString& maaNimi = maaData.englanniksi();


    if( maa == "gb" || maa=="in" || maa=="ir" || maa == "iq" || maa=="ie" || maa=="nz" || maa=="pk" ||
        maa == "ru" || maa=="sg" || maa=="kr" || maa == "lk" || maa=="th")
        return nimi + "\n" + osoite + "\n" + kaupunki + "\n" +
                (!osavaltio.isEmpty() ? osavaltio + "\n" : "" ) + postinumero + "\n" + maaNimi;
    else if(maa == "au" || maa == "ca" || maa == "tw" || maa == "us")
        return nimi + "\n" + osoite + "\n" + kaupunki +  " " + osavaltio + " " + postinumero + "\n" + maaNimi;
    else if( maa == "id")
        return nimi + "\n" + osoite + "\n" + kaupunki + "\n" + osavaltio + " " + postinumero + "\n" + maaNimi;


    // Yleinen osoitetyyppi on eurooppalainen ;)

    QString txt = nimi + "\n" +
                  osoite + "\n" +
                  (postinumero.isEmpty() ? "" : postinumero + " " )+ kaupunki;

    if( osavaltio.isEmpty())
        txt += "\n" + osavaltio;

    if( maa != "fi" ) {
        txt += "\n" + maaNimi;
    }
    return txt;
}

MaaModel::Maa MaaModel::maaKoodilla(const QString &koodi) const
{
    for(auto maa : maat_)
        if(maa.koodi() == koodi)
            return maa;
    return Maa("","","");
}

void MaaModel::lataa()
{
    QFile maat(":/rekisteri/maat.csv");
    maat.open(QFile::ReadOnly);
    QTextStream in(&maat);
    in.setCodec("UTF-8");
    while(!in.atEnd()) {
        QStringList list = in.readLine().split(";");
        if( list.length() >= 3) {
            maat_.append(Maa(list.value(2).toLower(),
                             list.value(0),
                             list.value(1),
                             list.value(3)));
        }
    }
}


MaaModel* MaaModel::instanssi__ = nullptr;

MaaModel::Maa::Maa(const QString &koodi, const QString &nimi, const QString englanniksi, const QString &alvreg) :
        nimi_(nimi),
        koodi_(koodi),
        alvreg_(alvreg),
        englanniksi_(englanniksi)
{
    QIcon icon(":/liput/" + koodi + ".png");
    icon_ = icon.isNull() ? QIcon(":/pic/tyhja.png") : icon;
}
