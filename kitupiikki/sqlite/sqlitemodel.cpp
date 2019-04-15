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
#include "sqlitemodel.h"
#include "db/kirjanpito.h"

#include "sqliteyhteys.h"

#include <QSettings>
#include <QImage>

SQLiteModel::SQLiteModel(QObject *parent)
    : QAbstractListModel(parent)
{

}


int SQLiteModel::rowCount(const QModelIndex &parent) const
{
    if (parent.isValid())
        return 0;
    return viimeiset_.count();
}

QVariant SQLiteModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid())
        return QVariant();

    QVariantMap map = viimeiset_.at(index.row()).toMap();
    switch (role) {
    case Qt::DisplayRole:
    case NimiRooli:
        return map.value("nimi").toString();
    case Qt::DecorationRole:
        return(  QPixmap::fromImage( QImage::fromData(map.value("icon").toByteArray(),"PNG") ) );
    case PolkuRooli:
        {
            QString polku = map.value("polku").toString();
            // Palautetaan aina täydellinen polku
            QDir portableDir( kp()->portableDir() );
            if( !kp()->portableDir().isEmpty())
                polku = QDir::cleanPath(portableDir.absoluteFilePath(polku));
            return polku;
        }
    }

    return QVariant();
}

bool SQLiteModel::avaaTiedosto(const QString &polku, bool ilmoitavirheestaAvattaessa)
{
    SQLiteYhteys* yhteys = new SQLiteYhteys(this, polku);
    connect( yhteys, &SQLiteYhteys::yhteysAvattu, kp(), &Kirjanpito::yhteysAvattu);
    connect( yhteys, &SQLiteYhteys::yhteysAvattu, this, &SQLiteModel::lisaaViimeisiin);

    return yhteys->alustaYhteys(ilmoitavirheestaAvattaessa);
}

void SQLiteModel::lataaViimeiset()
{
    beginResetModel();
    viimeiset_ = kp()->settings()->value("ViimeTiedostot").toList();

    QDir portableDir( kp()->portableDir() );

    QMutableListIterator<QVariant> iter( viimeiset_ );
    while( iter.hasNext())
    {
        QString polku = iter.next().toMap().value("polku").toString();
        if( !kp()->portableDir().isEmpty())
            polku = QDir::cleanPath(portableDir.absoluteFilePath(polku));
        if( !QFile::exists(polku))
            iter.remove();
    }

    kp()->settings()->setValue("ViimeTiedostot", viimeiset_);
    endResetModel();

}

void SQLiteModel::poistaListalta(const QString &polku)
{

    QDir portableDir( kp()->portableDir() );
    QString poistettava = kp()->portableDir().isEmpty() ? polku : portableDir.relativeFilePath(polku);

    QMutableListIterator<QVariant> iter( viimeiset_ );
    while( iter.hasNext())
    {
        QString tamanpolku = iter.next().toMap().value("polku").toString();
        if( poistettava == tamanpolku )
            iter.remove();
    }
    kp()->settings()->setValue("ViimeTiedostot", viimeiset_);
    endResetModel();

}


void SQLiteModel::lisaaViimeisiin(bool onnistuiko)
{
    if( onnistuiko ) {
        SQLiteYhteys* yhteys = qobject_cast<SQLiteYhteys*>(sender());
        // Viimeisin poistetaan listalta, ja lisätään ensimmäiseksi

        beginResetModel();
        QMutableListIterator<QVariant> iter( viimeiset_ );
        while( iter.hasNext())
        {
            QString polku = iter.next().toMap().value("polku").toString();
            if( polku == yhteys->tiedostopolku())
                iter.remove();
        }

        QVariantMap map;

        // PORTABLE polut tallennetaan suhteessa portable-hakemistoon
        QDir portableDir( kp()->portableDir() );
        QString polku = yhteys->tiedostopolku();

        if( !kp()->portableDir().isEmpty())
            polku = portableDir.relativeFilePath(polku);

        map.insert("polku", yhteys->tiedostopolku() );
        map.insert("nimi", kp()->asetukset()->asetus("Nimi") );

        viimeiset_.insert(0, map);

        kp()->settings()->setValue("ViimeTiedostot", viimeiset_);
        endResetModel();
    }
}
