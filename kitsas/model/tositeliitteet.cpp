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


#include <QIcon>
#include <QFile>
#include <QMessageBox>
#include <QFileInfo>
#include <QMimeData>
#include <QUrl>

#include <QDebug>

#include "db/tositetyyppimodel.h"
#include "tuonti/pdftuonti.h"
#include "tuonti/csvtuonti.h"
#include "tuonti/titotuonti.h"

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

bool TositeLiitteet::lisaa(const QByteArray &sisalto, const QString& tiedostonnimi, const QString& rooli)
{
    if( sisalto.isNull())
        return false;

    QString nimi = tiedostonnimi;

    beginInsertRows( QModelIndex(), liitteet_.count(), liitteet_.count() );
    liitteet_.append( TositeLiite(0, nimi, sisalto, rooli) );
    endInsertRows();

    return true;
}

bool TositeLiitteet::lisaaTiedosto(const QString &polku)
{
    return lisaa( lueTiedosto(polku), QFileInfo(polku).fileName());
}

bool TositeLiitteet::lisaaHeti(const QByteArray &liite, const QString &tiedostonnimi)
{
    if( liite.isNull())
        return false;

    beginInsertRows( QModelIndex(), liitteet_.count(), liitteet_.count() );
    liitteet_.append( TositeLiite(0, tiedostonnimi, liite) );
    int liiteIndeksi = liitteet_.count() - 1;
    endInsertRows();

    emit naytaliite( liite );

    KpKysely* liitekysely = kpk("/liitteet", KpKysely::POST);
    connect( liitekysely, &KpKysely::lisaysVastaus, [this, liiteIndeksi] (const QVariant&, int id) {
            this->liitteet_[liiteIndeksi].setLiitettava(id);
        });


    // Ensimmäisestä liitteestä tuodaan tiedot
    if( liitteet_.count() == 1) {

        QString tyyppi = KpKysely::tiedostotyyppi(liite);
        if( tyyppi == "application/pdf") {
            QVariant tuotu = Tuonti::PdfTuonti::tuo(liite);
            if( tuotu.toMap().value("tyyppi").toInt() == TositeTyyppi::TILIOTE) {
                KpKysely *kysely = kpk("/tuontitulkki", KpKysely::POST);
                connect( kysely, &KpKysely::vastaus, this, &TositeLiitteet::tuonti);
                kysely->kysy(tuotu);
            } else {
                emit this->tuonti( &tuotu);
            }
        } else if(  liite.startsWith("T00322100") ||  tyyppi == "text/csv") {
            QVariant tuotu = liite.startsWith("T00322100") ?
                        Tuonti::TitoTuonti::tuo(liite) :
                        Tuonti::CsvTuonti::tuo(liite);
            KpKysely *kysely = kpk("/tuontitulkki", KpKysely::POST);
            connect( kysely, &KpKysely::vastaus, this, &TositeLiitteet::tuonti);
            kysely->kysy(tuotu);
        }
    }

    QMap<QString,QString> meta;
    meta.insert("Filename", tiedostonnimi);
    liitekysely->lahetaTiedosto(liite, meta);
    return true;

}

bool TositeLiitteet::lisaaHetiTiedosto(const QString &polku)
{
    return lisaaHeti( lueTiedosto(polku), QFileInfo(polku).fileName() );
}


bool TositeLiitteet::canDropMimeData(const QMimeData *data, Qt::DropAction /*action*/, int /* row */, int /*column*/, const QModelIndex &/*parent*/) const
{
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

                lisaaHetiTiedosto(info.absoluteFilePath());
                lisatty++;
            }
        }
    }
    if( lisatty)
        return true;

    if( !lisatty && data->formats().contains("image/jpg"))
    {
        lisaaHeti( data->data("image/jpg"), tr("liite.jpg") );
        return true;
    }
    else if(!lisatty && data->formats().contains("image/png"))
    {
        lisaaHeti( data->data("image/png"), tr("liite.png") );
        return true;
    }

    return false;
}

int TositeLiitteet::tallennettaviaLiitteita() const
{
    int maara = 0;
    for(auto liite : liitteet_)
        if( !liite.getLiiteId() && !liite.getLiitettava())
            maara++;

    return maara;
}

void TositeLiitteet::tallennaLiitteet(int tositeId)
{
    tositeId_ = tositeId;
    tallennuksessa_ = -1;
    tallennaSeuraava();
}

QVariantList TositeLiitteet::liitettavat() const
{
    QVariantList lista;
    for( auto liite: liitteet_)
        if( liite.getLiitettava())
            lista.append(liite.getLiiteId());
    return lista;
}

void TositeLiitteet::nayta(int indeksi)
{
    if( indeksi < 0)
        emit naytaliite( QByteArray());
    else {
        QByteArray sisalto = liitteet_.at(indeksi).getSisalto();
        if(sisalto.isEmpty()) {
            KpKysely* kysely = kpk(QString("/liitteet/%1").arg( liitteet_.at(indeksi).getLiiteId() ));
            connect( kysely, &KpKysely::vastaus, this, &TositeLiitteet::liitesaapuu);
            kysely->kysy();
        } else {
            emit ( naytaliite(sisalto) );
        }
    }
}

void TositeLiitteet::poista(int indeksi)
{
    if( liitteet_.at(indeksi).getLiiteId()) {
        // Liite on tallennettu
        KpKysely* poisto = kpk( QString("/liitteet/%1").arg( liitteet_.at(indeksi).getLiiteId() ), KpKysely::DELETE);
        poisto->kysy();
    }
    beginRemoveRows(QModelIndex(),indeksi, indeksi);
    liitteet_.removeAt(indeksi);
    endRemoveRows();
}

void TositeLiitteet::tallennaSeuraava()
{
    while( tallennuksessa_ < liitteet_.count() - 1)
    {
        tallennuksessa_++;
        if( !liitteet_.at(tallennuksessa_).getLiiteId() && !liitteet_.at(tallennuksessa_).getLiitettava())
        {
            KpKysely* tallennus = nullptr;
            if( liitteet_.at(tallennuksessa_).getRooli().isEmpty())
                tallennus = kpk( QString("/liitteet/%1").arg(tositeId_), KpKysely::POST );
            else
                tallennus = kpk( QString("/liitteet/%1/%2").arg(tositeId_).arg(liitteet_.at(tallennuksessa_).getRooli()), KpKysely::PUT);

            connect( tallennus, &KpKysely::vastaus, this, &TositeLiitteet::tallennaSeuraava);
            QMap<QString,QString> meta;
            meta.insert("Filename", liitteet_.at(tallennuksessa_).getNimi());
            tallennus->lahetaTiedosto( liitteet_.at(tallennuksessa_).getSisalto(), meta );
            return;
        }
    }
    emit liitteetTallennettu();
}

void TositeLiitteet::liitesaapuu(QVariant *data)
{
    // Tässä voisi myös laittaa liitteen muistiin ;)
    emit naytaliite( data->toByteArray() );
}

QByteArray TositeLiitteet::lueTiedosto(const QString &polku)
{
    QByteArray ba;
    QFile tiedosto(polku);
    if( !tiedosto.open(QIODevice::ReadOnly) )
    {
        QMessageBox::critical(nullptr, tr("Tiedostovirhe"),
                              tr("Tiedoston %1 avaaminen epäonnistui \n%2").arg(polku).arg(tiedosto.errorString()));
        return QByteArray();
    }

    ba = tiedosto.readAll();
    tiedosto.close();
    return ba;
}

// ************************************ TOSITELIITE **********************************

TositeLiitteet::TositeLiite::TositeLiite(int id, const QString &nimi, const QByteArray &sisalto, const QString &rooli) :
    liiteId_(id),
    nimi_(nimi),
    sisalto_(sisalto),
    rooli_(rooli)
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

QString TositeLiitteet::TositeLiite::getRooli() const
{
    return rooli_;
}

void TositeLiitteet::TositeLiite::setRooli(const QString &rooli)
{
    rooli_ = rooli;
}

bool TositeLiitteet::TositeLiite::getLiitettava() const
{
    return liitettava_;
}

void TositeLiitteet::TositeLiite::setLiitettava(int id)
{
    liitettava_ = true;
    liiteId_ = id;
}
