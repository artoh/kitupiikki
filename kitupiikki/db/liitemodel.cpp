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
#include <QPdfWriter>
#include <QMessageBox>
#include <QMimeData>

#include <QPrinter>
#include <QPainter>
#include <QImage>
#include <QSettings>

#include <QBuffer>

#include "liitemodel.h"
#include "tositemodel.h"
#include "kirjanpito.h"

#include <QDebug>
#include <QSqlError>

#include <poppler/qt5/poppler-qt5.h>


LiiteModel::LiiteModel(TositeModel *tositemodel, QObject *parent)
    : QAbstractListModel(parent), tositeModel_(tositemodel), muokattu_(false)
{
    if( !tositemodel)
        lataa();
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
    else if( role == Sharooli)
        return QVariant( liite.sha);
    else if( role == TiedostoNimiRooli && tositeModel_)
    {
        if( liite.pdf.startsWith("%PDF") )
        {
            return QString("%1-%2.pdf")
                    .arg( tositeModel_->id(), 8, 10, QChar('0') )
                    .arg( liite.liiteno , 2, 10, QChar('0') );
        }
        else if( liite.pdf.startsWith(  static_cast<char>( 0xff) ))
        {
            return QString("%1-%2.png")
                    .arg( tositeModel_->id(), 8, 10, QChar('0') )
                    .arg( liite.liiteno , 2, 10, QChar('0') );
        }
        else
        {
            return QString("%1-%2-%3")
                    .arg( tositeModel_->id(), 8, 10, QChar('0') )
                    .arg( liite.liiteno , 2, 10, QChar('0'))
                    .arg( liite.otsikko );
        }
    }
    else if( role == PdfRooli )
        return liite.pdf;
    else if( role == LiiteNumeroRooli )
        return liite.liiteno;
    else if( role == IdRooli)
        return liite.id;

    else if( role == Qt::DecorationRole)
    {
        if( liite.thumbnail.isEmpty())
            return QIcon(":/pic/tekstisivu.png");

        QPixmap pixmap;
        pixmap.loadFromData( liite.thumbnail, "PNG");
        return QIcon( pixmap );
    }

    return QVariant();
}

Qt::ItemFlags LiiteModel::flags(const QModelIndex &index) const
{
    if( tositeModel_ && tositeModel_->muokkausSallittu())
        return QAbstractListModel::flags(index) | Qt::ItemIsEditable | Qt::ItemIsDropEnabled;
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
        emit liiteMuutettu();
        return true;
    }
    return false;
}


int LiiteModel::lisaaLiite(const QByteArray &liite, const QString &otsikko, const QString &polusta)
{
    beginInsertRows( QModelIndex(), liitteet_.count(), liitteet_.count() );
    Liite uusi;

    uusi.liiteno = seuraavaNumero();
    uusi.pdf = liite;
    uusi.otsikko = otsikko;
    uusi.muokattu = true;
    uusi.lisattyPolusta = polusta;

    if( liite.startsWith("%PDF") &&  !kp()->settings()->value("PopplerPois").toBool() )
    {

        // Peukkukuvan muodostaminen
        Poppler::Document *pdfDoc = Poppler::Document::loadFromData( liite );
        if( pdfDoc )
        {
            Poppler::Page *pdfsivu = pdfDoc->page(0);
            if( pdfsivu )
            {
                QImage image = pdfsivu->renderToImage(24,24);
                QPixmap kuva = QPixmap::fromImage( image.scaled(64,64,Qt::KeepAspectRatio) );
                QBuffer buffer(&uusi.thumbnail);
                buffer.open(QIODevice::WriteOnly);
                kuva.save(&buffer, "PNG");

                delete pdfsivu;
            }
            delete pdfDoc;
        }
    }
    else if( liite.startsWith(  static_cast<char>( 0xff) ))
    {
        // Peukkukuvan muodostaminen jpg-tiedostosta
        QImage kuva = QImage::fromData( liite, "JPG" );
        if( !kuva.isNull())
        {
            QPixmap peukkukuva = QPixmap::fromImage( kuva.scaled(64,64,Qt::KeepAspectRatio) );
            QBuffer buffer(&uusi.thumbnail);
            buffer.open(QIODevice::WriteOnly);
            peukkukuva.save(&buffer, "PNG");
        }
    }

    liitteet_.append(uusi);

    endInsertRows();
    muokattu_ = true;
    emit liiteMuutettu();

    return uusi.liiteno;
}

int LiiteModel::asetaLiite(const QByteArray &liite, const QString &otsikko)
{
    for(int i=0; i < liitteet_.count(); i++)
        if( liitteet_.at(i).otsikko == otsikko)
        {
            poistaLiite(i);
            break;
        }
    return lisaaLiite(liite, otsikko);
}

int LiiteModel::lisaaTiedosto(const QString &polku, const QString &otsikko)
{
    QByteArray data;

    QFile tiedosto(polku);
    if( !tiedosto.open(QIODevice::ReadOnly) )
    {
        QMessageBox::critical(nullptr, tr("Tiedostovirhe"),
                              tr("Tiedoston %1 avaaminen epäonnistui \n%2").arg(polku).arg(tiedosto.errorString()));
        return 0;
    }
    data = tiedosto.readAll();
    tiedosto.close();

    if( !data.startsWith("%PDF"))
    {
        // Kuvatiedostot muunnetaan jpg-muotoon
        QByteArray jpg;
        QImage kuva(polku);
        if( !kuva.isNull())
        {

            QBuffer puskuri(&jpg);
            puskuri.open(QBuffer::WriteOnly);
            kuva.save(&puskuri, "JPG");

            puskuri.close();
            return lisaaLiite(jpg, otsikko, polku);
        }
    }

    return lisaaLiite(data, otsikko, polku);
}

void LiiteModel::poistaLiite(int indeksi)
{
    beginRemoveRows( QModelIndex(), indeksi, indeksi);
    if( liitteet_[indeksi].id)
        poistetutIdt_.append( liitteet_[indeksi].id);

    liitteet_.removeAt(indeksi);
    endRemoveRows();

    muokattu_ = true;
    emit liiteMuutettu();
}

QByteArray LiiteModel::liite(const QString &otsikko)
{
    for( Liite liite : liitteet_ )
        if( liite.otsikko == otsikko )
            return liite.pdf;

    return QByteArray();
}

bool LiiteModel::canDropMimeData(const QMimeData *data, Qt::DropAction /*action*/, int /* row */, int /*column*/, const QModelIndex &/*parent*/) const
{
    return( data->hasUrls() || data->formats().contains("image/jpg") || data->formats().contains("image/png"));
}

bool LiiteModel::dropMimeData(const QMimeData *data, Qt::DropAction action, int /* row */, int /* column */, const QModelIndex & /* parent */)
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

                lisaaTiedosto(info.absoluteFilePath(), info.fileName());
                lisatty++;
            }
        }
    }
    if( lisatty)
        return true;

    if( !lisatty && data->formats().contains("image/jpg"))
    {
        lisaaLiite( data->data("image/jpg"), tr("liite.jpg") );
        return true;
    }
    else if(!lisatty && data->formats().contains("image/png"))
    {
        lisaaLiite( data->data("image/png"), tr("liite.png") );
        return true;
    }

    return false;
}


void LiiteModel::lataa()
{
    endResetModel();
    liitteet_.clear();

    QSqlQuery kysely( *kp()->tietokanta() );

    if( tositeModel_ )
        kysely.exec( QString("SELECT id, liiteno, otsikko, peukku, sha, data "
                         "FROM liite WHERE tosite=%1 ORDER BY liiteno").arg( tositeModel_->id() ));
    else
        kysely.exec( QString("SELECT id, liiteno, otsikko, peukku, sha, data "
                         "FROM liite WHERE tosite is NULL ORDER BY liiteno"));


    while( kysely.next())
    {
        Liite uusi;
        uusi.id = kysely.value("id").toInt();
        uusi.liiteno = kysely.value("liiteno").toInt();
        uusi.otsikko = kysely.value("otsikko").toString();
        uusi.sha = kysely.value("sha").toByteArray();
        uusi.thumbnail = kysely.value("peukku").toByteArray();
        uusi.pdf = kysely.value("data").toByteArray();

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

bool LiiteModel::tallenna()
{
    QString inboxPolku = kp()->asetukset()->asetus("KirjattavienKansio");

    QSqlQuery kysely( *kp()->tietokanta() );
    for( int i=0; i<liitteet_.count(); i++)
    {        
        if( liitteet_.at(i).muokattu)
        {
            if( liitteet_.at(i).id == 0)
            {

                liitteet_[i].sha = QCryptographicHash::hash( liitteet_.at(i).pdf, QCryptographicHash::Sha256).toHex();

                kysely.prepare("INSERT INTO liite(liiteno, tosite, otsikko, peukku, sha, data, liitetty) "
                               "VALUES(:liiteno, :tosite, :otsikko, :peukku, :sha, :data, :liitetty)");

                kysely.bindValue(":liiteno", liitteet_.at(i).liiteno);

                if( tositeModel_)
                    kysely.bindValue(":tosite", tositeModel_->id());
                else
                    kysely.bindValue(":tosite", QVariant());

                kysely.bindValue(":sha", liitteet_.at(i).sha);
                kysely.bindValue(":peukku", liitteet_.at(i).thumbnail);
                kysely.bindValue(":otsikko", liitteet_[i].otsikko);
                kysely.bindValue(":data", liitteet_.at(i).pdf);
                kysely.bindValue(":liitetty", QDateTime::currentDateTime());

                if( !kysely.exec() )
                    return false;
                liitteet_[i].id = kysely.lastInsertId().toInt();

                if( !inboxPolku.isEmpty() && liitteet_.at(i).lisattyPolusta.startsWith( inboxPolku ) )
                    QFile::remove( liitteet_.at(i).lisattyPolusta );

            }
            else
            {
                kysely.prepare("UPDATE liite SET otsikko=:otsikko WHERE id=:id");
                kysely.bindValue(":id", liitteet_.at(i).id);
                kysely.bindValue(":otsikko", liitteet_[i].otsikko);
                if( !kysely.exec() )
                    return false;
            }
            liitteet_[i].muokattu = false;
        }
    }

    // Poistetut liitteet
    for( int poistettuId : poistetutIdt_)
        kysely.exec( QString("DELETE from liite WHERE id=%1").arg(poistettuId) );

    muokattu_ = false;
    return true;
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

