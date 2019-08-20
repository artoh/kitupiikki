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
#include "db/kirjanpito.h"

#include "tuonti/tuonti.h"

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

    if( liitteet_.count())
        nayta(0);
    else
        emit naytaliite(QByteArray());
}

void TositeLiitteet::clear()
{
    beginResetModel();
    liitteet_.clear();
    endResetModel();
    emit naytaliite(QByteArray());
}

bool TositeLiitteet::lisaa(const QByteArray &sisalto, const QString &nimi)
{
    beginInsertRows( QModelIndex(), liitteet_.count(), liitteet_.count() );
    liitteet_.append( TositeLiite(0, nimi, sisalto) );
    endInsertRows();

    if( liitteet_.count() > 1)
        return true;    // Vain eka liite tuodaan

    // Toistaikseksi jpg nimellä
    if( nimi.endsWith(".jpg") )
    {
        KpKysely* liitekysely = kpk("/liitteet", KpKysely::POST);
        connect( liitekysely, &KpKysely::vastaus, [this] (QVariant *data)  { emit this->tuonti(data); });
        liitekysely->lahetaTiedosto(sisalto, nimi);
        return true;
    }

    // Käsitellään tuonti
    QVariant tuonnit = Tuonti::tuo(sisalto);
    qDebug() << tuonnit;


    KpKysely* kysely = kpk("/tuontitulkki", KpKysely::POST);
    connect( kysely, &KpKysely::vastaus, [this] (QVariant *data)  { emit this->tuonti(data); });
    kysely->kysy( tuonnit) ;

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

void TositeLiitteet::tallennaLiitteet(int tositeId)
{
    tositeId_ = tositeId;
    tallennuksessa_ = -1;
    tallennaSeuraava();
}

void TositeLiitteet::nayta(int indeksi)
{
    QByteArray sisalto = liitteet_.at(indeksi).getSisalto();
    if(sisalto.isEmpty()) {
        KpKysely* kysely = kpk(QString("/liitteet/%1").arg( liitteet_.at(indeksi).getLiiteId() ));
        connect( kysely, &KpKysely::vastaus, this, &TositeLiitteet::liitesaapuu);
        kysely->kysy();
    } else {
        emit ( naytaliite(sisalto) );
    }

}

void TositeLiitteet::tallennaSeuraava()
{
    while( tallennuksessa_ < liitteet_.count() - 1)
    {
        tallennuksessa_++;
        if( !liitteet_.at(tallennuksessa_).getLiiteId())
        {
            KpKysely* tallennus = kpk( QString("/liitteet/%1").arg(tositeId_), KpKysely::POST );
            connect( tallennus, &KpKysely::vastaus, this, &TositeLiitteet::tallennaSeuraava);
            tallennus->lahetaTiedosto( liitteet_.at(tallennuksessa_).getSisalto(), liitteet_.at(tallennuksessa_).getNimi() );
            return;
        }
    }
    // Kun tänne tullaan, on kaikki jo tallennettu
    // Jos on poistettavia, poistetaan ne

    if( poistetut_.count() )
    {
        KpKysely* poisto = kpk( QString("/liitteet/%1").arg( poistetut_.takeFirst() ), KpKysely::DELETE);
        connect( poisto, &KpKysely::vastaus, this, &TositeLiitteet::tallennaSeuraava);
        poisto->kysy();
        return;
    }

    // Koko tallennus valmis

    emit liitteetTallennettu();
}

void TositeLiitteet::liitesaapuu(QVariant *data)
{
    // Tässä voisi myös laittaa liitteen muistiin ;)
    emit naytaliite( data->toByteArray() );
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

QByteArray TositeLiitteet::TositeLiite::getSisalto() const
{
    return sisalto_;
}
