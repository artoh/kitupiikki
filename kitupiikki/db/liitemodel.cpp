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

#include <QPrinter>
#include <QTemporaryFile>
#include <QPainter>
#include <QImage>

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
    else if( role == TiedostoNimiRooli )
        return liiteNimi( liite.liiteno );
    else if( role == PolkuRooli)
        return liitePolku( liite.liiteno);
    else if( role == PdfRooli )
        return liite.pdf;

    else if( role == Qt::DecorationRole)
    {
        QPixmap pixmap;
        pixmap.loadFromData( liite.thumbnail, "PNG");
        return QIcon( pixmap );
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


    if( polku.toLower().endsWith(".pdf"))
    {
        // On valmiiksi pdf
        QFile tiedosto(polku);
        tiedosto.open( QIODevice::ReadOnly);

        uusi.pdf = tiedosto.readAll();

        tiedosto.close();
    }
    else
    {
        QImage kuva(polku);

        // Kuvatiedosto, muutetaan pdf-muotoon
        QTemporaryFile tempFile(QDir::tempPath() + "/imgcnv-XXXXXX.pdf");
        tempFile.open();
        tempFile.close();

        QPrinter printer(QPrinter::HighResolution);
        printer.setOutputFileName( tempFile.fileName());
        {
            QPainter painter( &printer);

            QRect rect = painter.viewport();
            QSize size = kuva.size();
            size.scale( rect.size(), Qt::KeepAspectRatio );
            painter.setViewport( rect.x(), rect.y(),
                                 size.width(), size.height());
            painter.setWindow( kuva.rect() );
            painter.drawImage(0,0,kuva);
        }

        QFile luku( tempFile.fileName());
        luku.open( QIODevice::ReadOnly );
        uusi.pdf = luku.readAll();
        luku.close();
    }

    uusi.otsikko = otsikko;
    uusi.muokattu = true;   // Uusi, tallentamaton tiedosto

    // Peukkukuvan tulostus, kun päästään sinne saakka
    Poppler::Document *pdfDoc = Poppler::Document::loadFromData( uusi.pdf );
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
    QFile( liitePolku( liitteet_.at(indeksi).liiteno ) ).remove();

    liitteet_.removeAt(indeksi);
    endRemoveRows();
}

QString LiiteModel::liitePolku(int liitenro) const
{
    return liitePolulla( tositeModel_->id(), liitenro );
}

QString LiiteModel::liiteNimi(int liitenro) const
{
    return QString("%1-%2.pdf")
            .arg( tositeModel_->id(), 8, 10, QChar('0') )
            .arg( liitenro, 2, 10, QChar('0') );
}

QString LiiteModel::liitePolulla(int tositeId, int liiteId)
{
    QString tiedostonnimi = QString("%1-%2.pdf")
            .arg( tositeId, 8, 10, QChar('0') )
            .arg( liiteId, 2, 10, QChar('0') );
    return liiteNimella( tiedostonnimi);
}

QString LiiteModel::liiteNimella(const QString &tiedosto)
{
    return kp()->hakemisto().absoluteFilePath( "liitteet/" + tiedosto);
}

void LiiteModel::lataa()
{
    endResetModel();
    liitteet_.clear();

    QSqlQuery kysely( *tositeModel_->tietokanta());
    kysely.exec( QString("SELECT id, liiteno, otsikko, peukku, sha "
                         "FROM liite WHERE tosite=%1 ORDER BY liiteno").arg( tositeModel_->id() ));
    while( kysely.next())
    {
        Liite uusi;
        uusi.id = kysely.value("id").toInt();
        uusi.liiteno = kysely.value("liiteno").toInt();
        uusi.otsikko = kysely.value("otsikko").toString();
        uusi.sha = kysely.value("sha").toByteArray();
        uusi.thumbnail = kysely.value("peukku").toByteArray();

        QFile tiedosto( liitePolku( uusi.liiteno) );
        tiedosto.open(QIODevice::ReadOnly);
        uusi.pdf = tiedosto.readAll();
        tiedosto.close();

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
        if( liitteet_.at(i).muokattu)
        {
            if( liitteet_.at(i).id == 0)
            {

                // Tallennetaan tiedosto
                QFile uusitiedosto( liitePolku( liitteet_.at(i).liiteno ) );
                uusitiedosto.open(QIODevice::WriteOnly);
                uusitiedosto.write( liitteet_.at(i).pdf );
                uusitiedosto.close();

                liitteet_[i].sha = QCryptographicHash::hash( liitteet_.at(i).pdf, QCryptographicHash::Sha256);

                kysely.prepare("INSERT INTO liite(liiteno, tosite, otsikko, peukku, sha) "
                               "VALUES(:liiteno, :tosite, :otsikko, :peukku, :sha)");

                kysely.bindValue(":liiteno", liitteet_.at(i).liiteno);
                kysely.bindValue(":tosite", tositeModel_->id());
                kysely.bindValue(":sha", liitteet_.at(i).sha);
                kysely.bindValue(":peukku", liitteet_.at(i).thumbnail);
            }
            else
            {
                kysely.prepare("UPDATE liite SET otsikko=:otsikko WHERE id=:id");
                kysely.bindValue(":id", liitteet_.at(i).id);
            }
            kysely.bindValue(":otsikko", liitteet_[i].otsikko);
            kysely.exec();
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

