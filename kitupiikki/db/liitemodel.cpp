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

#include <QSqlQuery>
#include <QByteArray>
#include <QCryptographicHash>
#include <QFile>
#include <QFileInfo>
#include <QIcon>

#include "liitemodel.h"
#include "tositemodel.h"
#include "kirjanpito.h"

#include <QDebug>
#include <QSqlError>

QString Liite::tiedostopolku() const
{
    if( uusitiedosto.isEmpty())
        return kp()->hakemisto().absoluteFilePath(QString("liitteet/%1").arg(tiedostonnimi));
    else
        return uusitiedosto;
}

QString Liite::tarkenne() const
{
    QString apu;
    if( uusitiedosto.isEmpty())
        apu = tiedostonnimi;
    else
        apu = uusitiedosto;

    int indeksi = apu.lastIndexOf('.');

    if( indeksi > 0)
    {
        return apu.mid( indeksi + 1).toLower();
    }
    return QString();
}


LiiteModel::LiiteModel(TositeModel *tositemodel, QObject *parent)
    : QAbstractListModel(parent), tositeModel_(tositemodel), muokattu_(false)
{

}

int LiiteModel::rowCount(const QModelIndex & /* parent */) const
{
    return liitteet_.count();
}

QVariant LiiteModel::data(const QModelIndex &index, int role) const
{
    if( !index.isValid())
        return QVariant();
    Liite liite = liitteet_.value(index.row());

    if( role == Qt::DisplayRole || role == OtsikkoRooli)
        return QVariant( liite.otsikko );
    else if( role == Polkurooli )
        return QVariant( liite.tiedostopolku() );
    else if( role == Tarkennerooli )
        return QVariant( liite.tarkenne() );
    else if( role == Sharooli)
        return QVariant( liite.sha);
    else if( role == TiedostoNimiRooli )
        return liite.tiedostonnimi;

    else if( role == Qt::DecorationRole)
    {

        if( liite.tarkenne() == "pdf")
            return QIcon(":/pic/pdf.png");
        else if( liite.tarkenne() == "jpg" || liite.tarkenne() == "jpeg" || liite.tarkenne() == "png")
            return QIcon( liite.tiedostopolku() );
        else
            return QIcon(":/pic/Possu64.png");
    }

    return QVariant();
}

Qt::ItemFlags LiiteModel::flags(const QModelIndex &index) const
{
    if( tositeModel_->muokkausSallittu())
        return QAbstractListModel::flags(index) | Qt::ItemIsEditable;
    else
        return QAbstractListModel::flags(index);
}

bool LiiteModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
    if( index.isValid() && role == Qt::EditRole)
    {
        // Vaihdetaan näytettävä nimi
        liitteet_[index.row()].otsikko = value.toString();
        liitteet_[index.row()].muokattu = true;
        return true;
    }
    return false;
}

void LiiteModel::lisaaTiedosto(const QString &polku, const QString &otsikko)
{
    beginInsertRows( QModelIndex(), liitteet_.count(), liitteet_.count() );
    Liite uusi;

    uusi.liiteno = seuraavaNumero();
    uusi.uusitiedosto = polku;
    uusi.otsikko = otsikko;
    uusi.muokattu = true;

    liitteet_.append(uusi);

    endInsertRows();
    muokattu_ = true;
}

void LiiteModel::poistaLiite(int indeksi)
{
    beginRemoveRows( QModelIndex(), indeksi, indeksi);
    if( liitteet_[indeksi].id)
        poistetutIdt_.append( liitteet_[indeksi].id);

    // Poistetaan tiedosto
    QFile( liitteet_[indeksi].tiedostopolku() ).remove();

    liitteet_.removeAt(indeksi);
    endRemoveRows();
}

void LiiteModel::lataa()
{
    endResetModel();
    liitteet_.clear();

    QSqlQuery kysely( *tositeModel_->tietokanta());
    kysely.exec( QString("SELECT id, liiteno, otsikko, tiedosto, sha "
                         "FROM liite WHERE tosite=%1 ORDER BY liiteno").arg( tositeModel_->id() ));
    while( kysely.next())
    {
        Liite uusi;
        uusi.id = kysely.value("id").toInt();
        uusi.liiteno = kysely.value("liiteno").toInt();
        uusi.otsikko = kysely.value("otsikko").toString();
        uusi.tiedostonnimi = kysely.value("tiedosto").toString();
        uusi.sha = kysely.value("sha").toByteArray();
        liitteet_.append(uusi);
    }
    endResetModel();
    muokattu_ = false;
}

void LiiteModel::tyhjaa()
{
    beginResetModel();
    liitteet_.clear();
    endResetModel();
    muokattu_ = false;
}

void LiiteModel::tallenna()
{
    QSqlQuery kysely( *tositeModel_->tietokanta());
    for( int i=0; i<liitteet_.count(); i++)
    {
        // Ensin mahdollinen tiedoston kopiointi
        if( !liitteet_[i].uusitiedosto.isEmpty())
        {
            // Tiedoston nimi 00000001-01.jpg
            // Tositteen nro - Liitteen nro . tarkenne
            QFileInfo info( liitteet_[i].uusitiedosto );
            QString uusinimi = QString("%1-%2.%3")
                    .arg( tositeModel_->id(), 8, 10, QChar('0') )
                    .arg( liitteet_[i].liiteno, 2, 10, QChar('0'))
                    .arg( info.suffix().toLower() );

            // Kopioidaan liitteet/ -hakemistoon
            QString kopiopolku = kp()->hakemisto().absoluteFilePath("liitteet/" + uusinimi);

            QFile tiedosto( liitteet_[i].uusitiedosto);
            tiedosto.open( QIODevice::ReadOnly);
            QByteArray sisalto = tiedosto.readAll();
            tiedosto.close();

            // Lasketaan SHA256-tiiviste eheyden varmistamiseksi
            QCryptographicHash hash(QCryptographicHash::Sha256);
            hash.addData(sisalto);
            liitteet_[i].sha = hash.result().toHex();

            // Kirjoitetaan tiedosto uuteen hakemistoonsa
            QFile uusitiedosto(kopiopolku);
            uusitiedosto.open(QIODevice::WriteOnly);
            uusitiedosto.write(sisalto);
            uusitiedosto.close();
            liitteet_[i].tiedostonnimi = uusinimi;
            liitteet_[i].uusitiedosto = QString();
        }


        if( liitteet_[i].muokattu && !liitteet_[i].tiedostonnimi.isEmpty())
        {
            if( liitteet_[i].id)
            {
                kysely.prepare("UPDATE liite SET otsikko=:otsikko, tiedosto=:tiedosto,"
                               "sha=:sha WHERE id=:id");
                kysely.bindValue(":id", liitteet_[i].id);
            }
            else
            {
                kysely.prepare("INSERT INTO liite(liiteno, tosite, otsikko, tiedosto, sha) "
                               "VALUES(:liiteno, :tosite, :otsikko, :tiedosto, :sha)");
                kysely.bindValue(":liiteno", liitteet_[i].liiteno);
                kysely.bindValue(":tosite", tositeModel_->id());
            }

            kysely.bindValue(":otsikko", liitteet_[i].otsikko);
            kysely.bindValue(":tiedosto", liitteet_[i].tiedostonnimi);
            kysely.bindValue(":sha", liitteet_[i].sha);

            if( kysely.exec())
                liitteet_[i].muokattu = false;
            else
                qDebug() << kysely.lastQuery() << " " << kysely.lastError().text();

            if( !liitteet_[i].id)
                liitteet_[i].id = kysely.lastInsertId().toInt();
        }
    }
    muokattu_ = false;
}

int LiiteModel::seuraavaNumero() const
{
    int seuraava = 1;
    foreach (Liite liite, liitteet_)
    {
        if( liite.liiteno >= seuraava)
            seuraava = liite.liiteno + 1;
    }
    return seuraava;
}

