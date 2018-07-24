/*
   Copyright (C) 2018 Arto Hyvättinen

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
#include "inboxlista.h"

#include "db/kirjanpito.h"

#include <QFileSystemWatcher>
#include <QListWidgetItem>
#include <QDrag>
#include <QMimeData>
#include <QApplication>

InboxLista::InboxLista()
{
    vahti_ = new QFileSystemWatcher(this);
    connect( kp(), &Kirjanpito::inboxMuuttui, this, &InboxLista::alusta);
    connect( kp(), &Kirjanpito::tietokantaVaihtui, this, &InboxLista::alusta);
    connect( vahti_, &QFileSystemWatcher::directoryChanged, this, &InboxLista::paivita);

}

void InboxLista::alusta()
{
    if( !polku_.isEmpty())
        vahti_->removePath(polku_);
    polku_ = kp()->asetukset()->asetus("KirjattavienKansio");
    if( !polku_.isEmpty())
        vahti_->addPath(polku_);

    paivita();
}

void InboxLista::paivita()
{
    clear();

    if( polku_.isEmpty())
    {
        emit nayta(false);
        return;
    }

    QDir dir( polku_ );
    dir.setFilter(QDir::Files);
    QFileInfoList list = dir.entryInfoList();
    for( QFileInfo info : list)
    {
        QString tiedostonimi = info.fileName().toLower();
        if( tiedostonimi.endsWith(".pdf")  || tiedostonimi.endsWith(".jpg") ||
            tiedostonimi.endsWith(".jpeg") || tiedostonimi.endsWith(".png"))
        {
            QListWidgetItem *item = new QListWidgetItem( info.fileName(), this );
            if( tiedostonimi.endsWith(".pdf"))
                item->setIcon(QIcon(":/pic/pdf.png"));
            else
                item->setIcon(QIcon(":/pic/kuva.png"));
            item->setData(Qt::UserRole, info.absoluteFilePath());
        }
    }

    emit nayta( count() > 0 );

}

void InboxLista::mousePressEvent(QMouseEvent *event)
{
    if( event->button() == Qt::LeftButton)
        alkuPos_ = event->pos();
    QListWidget::mousePressEvent(event);
}

void InboxLista::mouseMoveEvent(QMouseEvent *event)
{
    if( event->buttons() & Qt::LeftButton)
    {
        if(  ( event->pos() - alkuPos_).manhattanLength() >= QApplication::startDragDistance() )
            aloitaRaahaus();
    }
    QListWidget::mouseMoveEvent(event);
}

void InboxLista::aloitaRaahaus()
{
    QListWidgetItem *item = currentItem();
    if( item )
    {
        QMimeData *mimeData = new QMimeData;
        QList<QUrl> urlista;
        urlista.append( QUrl::fromLocalFile( item->data(Qt::UserRole).toString() ) );
        mimeData->setUrls( urlista );

        QDrag *drag = new QDrag(this);
        drag->setMimeData( mimeData );
        drag->setPixmap( item->icon().pixmap(32,32) );
        drag->exec(Qt::CopyAction);
    }
}
