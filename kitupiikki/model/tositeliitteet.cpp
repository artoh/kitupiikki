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
#include "tositeliitteet.h"

#include <QIcon>
#include <QFile>
#include <QMessageBox>
#include <QFileInfo>
#include <QMimeData>
#include <QUrl>

#include <QDebug>

TositeLiitteet::TositeLiitteet(QObject *parent)
    : QAbstractListModel(parent)
{
}



int TositeLiitteet::rowCount(const QModelIndex &parent) const
{
    if (parent.isValid())
        return 0;

    return liitteet_.count();
}


QVariant TositeLiitteet::data(const QModelIndex &index, int role) const
{
    if (!index.isValid())
        return QVariant();

    if( role == Qt::DisplayRole)
    {
        return liitteet_.at(index.row()).getNimi();
    }
    else if( role == Qt::DecorationRole)
        return QIcon(":/pic/tekstisivu.png");

    // FIXME: Implement me!
    return QVariant();
}

Qt::ItemFlags TositeLiitteet::flags(const QModelIndex &index) const
{
    return QAbstractListModel::flags(index) | Qt::ItemIsDropEnabled;

}

void TositeLiitteet::lataa(QVariantList data)
{
    beginResetModel();
    liitteet_.clear();

    for( QVariant item : data) {
        QVariantMap map = item.toMap();

        liitteet_.append( TositeLiite( map.value("id").toInt(),
                                       map.value("nimi").toString()) );
    }
    endResetModel();
}

bool TositeLiitteet::lisaa(const QByteArray &sisalto, const QString &nimi)
{
    beginInsertRows( QModelIndex(), liitteet_.count(), liitteet_.count() );
    liitteet_.append( TositeLiite(0, nimi, sisalto) );
    endInsertRows();

    return true;
}

bool TositeLiitteet::lisaaTiedosto(const QString &polku)
{
    QByteArray ba;
    QFile tiedosto(polku);
    if( !tiedosto.open(QIODevice::ReadOnly) )
    {
        QMessageBox::critical(nullptr, tr("Tiedostovirhe"),
                              tr("Tiedoston %1 avaaminen epäonnistui \n%2").arg(polku).arg(tiedosto.errorString()));
        return false;
    }

    ba = tiedosto.readAll();
    tiedosto.close();

    QFileInfo info(polku);

    return lisaa( ba, info.fileName() );
}


bool TositeLiitteet::canDropMimeData(const QMimeData *data, Qt::DropAction /*action*/, int /* row */, int /*column*/, const QModelIndex &/*parent*/) const
{
    qDebug() << "f" << data->formats() << " u " << data->hasUrls();

    return data->hasUrls() || data->formats().contains("image/jpg") || data->formats().contains("image/png");
}

bool TositeLiitteet::dropMimeData(const QMimeData *data, Qt::DropAction action, int /* row */, int /* column */, const QModelIndex & /* parent */)
{
    if( action == Qt::IgnoreAction)
        return true;

    int lisatty = 0;
    // Liitetiedosto pudotettu
    if( data->hasUrls())
    {
        QList<QUrl> urlit = data->urls();
        for(auto url : urlit)
        {
            if( url.isLocalFile())
            {
                QFileInfo info( url.toLocalFile());
                QString polku = info.absoluteFilePath();

                lisaaTiedosto(info.absoluteFilePath());
                lisatty++;
            }
        }
    }
    if( lisatty)
        return true;

    if( !lisatty && data->formats().contains("image/jpg"))
    {
        lisaa( data->data("image/jpg"), tr("liite.jpg") );
        return true;
    }
    else if(!lisatty && data->formats().contains("image/png"))
    {
        lisaa( data->data("image/png"), tr("liite.png") );
        return true;
    }

    return false;
}

int TositeLiitteet::tallennettaviaLiitteita() const
{
    int maara = poistetut_.count();
    for(auto liite : liitteet_)
        if( !liite.getLiiteId() )
            maara++;

    return maara;
}

// ************************************ TOSITELIITE **********************************

TositeLiitteet::TositeLiite::TositeLiite(int id, const QString &nimi, const QByteArray &sisalto) :
    liiteId_(id),
    nimi_(nimi),
    sisalto_(sisalto)
{

}

int TositeLiitteet::TositeLiite::getLiiteId() const
{
    return liiteId_;
}

void TositeLiitteet::TositeLiite::setLiiteId(int value)
{
    liiteId_ = value;
}

QString TositeLiitteet::TositeLiite::getNimi() const
{
    return nimi_;
}

void TositeLiitteet::TositeLiite::setNimi(const QString &value)
{
    nimi_ = value;
}
