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

#include <QFileDialog>
#include <QFileInfo>

#include <QGraphicsScene>
#include <QGraphicsView>

#include <QDebug>

#include <QMimeData>
#include <QDragEnterEvent>
#include <QDragMoveEvent>
#include <QDropEvent>

#include <poppler/qt5/poppler-qt5.h>

#include <QGraphicsPixmapItem>

#include "naytaliitewg.h"
#include "db/kirjanpito.h"


NaytaliiteWg::NaytaliiteWg(QWidget *parent)
    : QStackedWidget(parent)
{
    QWidget *paasivu = new QWidget();
    ui = new Ui::TositeWg;
    ui->setupUi(paasivu);
    addWidget(paasivu);

    scene = new QGraphicsScene(this);
    view = new QGraphicsView(scene);

    addWidget(view);

    connect(ui->valitseTiedostoNappi, SIGNAL(clicked(bool)), this, SLOT(valitseTiedosto()));

    setAcceptDrops(true);
}

NaytaliiteWg::~NaytaliiteWg()
{
    delete ui;
}

void NaytaliiteWg::valitseTiedosto()
{
    QString polku = QFileDialog::getOpenFileName(this, tr("Valitse tosite"),QString(),tr("Kuvat (*.png *.jpg);; Pdf-tiedostot (*.pdf)"));
    if( !polku.isEmpty())
    {
        emit lisaaLiite( polku );
    }
}

void NaytaliiteWg::naytaTiedosto(const QString &polku)
{
    if( polku.isEmpty())
    {
        setCurrentIndex(0);
    }
    else
    {
        // Näytä tiedosto
        scene->clear();

        if( polku.toLower().endsWith(".pdf"))
        {
            // Näytä pdf
            Poppler::Document *pdfDoc = Poppler::Document::load( polku );
            if( !pdfDoc )
                return;

            double ypos = 0.0;
            // Monisivuisen pdf:n sivut pinotaan päällekkäin
            for( int sivu = 0; sivu < pdfDoc->numPages(); sivu++)
            {
                Poppler::Page *pdfSivu = pdfDoc->page(sivu);

                if( !pdfSivu )
                    continue;

                QImage image = pdfSivu->renderToImage(144.0, 144.0);
                QPixmap kuva = QPixmap::fromImage( image);

                QGraphicsPixmapItem *item = scene->addPixmap(kuva);
                item->setY( ypos );
                ypos += kuva.height();

                delete pdfSivu;
            }
            delete pdfDoc;

            view->fitInView(0, 0, view->scene()->sceneRect().width(), 0, Qt::KeepAspectRatio);
        }
        else
        {
                // Ladataan kuvatiedosto
                QPixmap kuva( polku );
                scene->addPixmap(kuva);
                // Skaalataan viewiä niin, että mahtuu sivusuunnassa
                view->fitInView(0,0,kuva.width(), kuva.width(), Qt::KeepAspectRatio);
        }

        // Tositteen näyttöwiget esiin
        setCurrentIndex(1);
    }
}

void NaytaliiteWg::dragEnterEvent(QDragEnterEvent *event)
{
    if( event->mimeData()->hasUrls())
        event->accept();
}

void NaytaliiteWg::dragMoveEvent(QDragMoveEvent *event)
{
    event->accept();
}

void NaytaliiteWg::dropEvent(QDropEvent *event)
{
    // Liitetiedosto pudotettu
    if( event->mimeData()->hasUrls())
    {
        QList<QUrl> urlit = event->mimeData()->urls();
        foreach (QUrl url, urlit)
        {
            qDebug() << url;
            if( url.isLocalFile())
                emit lisaaLiite( url.path() );
        }
    }
}


