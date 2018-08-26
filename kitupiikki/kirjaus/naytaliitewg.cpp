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
#include <QClipboard>

#include <poppler/qt5/poppler-qt5.h>

#include <QGraphicsPixmapItem>

#include "naytaliitewg.h"
#include "db/kirjanpito.h"

#include "naytin/naytinview.h"


NaytaliiteWg::NaytaliiteWg(QWidget *parent)
    : QStackedWidget(parent)
{
    QWidget *paasivu = new QWidget();
    ui = new Ui::TositeWg;
    ui->setupUi(paasivu);
    addWidget(paasivu);

    view = new NaytinView(this);

    view->setDragMode( QGraphicsView::ScrollHandDrag);

    addWidget(view);

    connect(ui->valitseTiedostoNappi, SIGNAL(clicked(bool)), this, SLOT(valitseTiedosto()));
    connect( qApp->clipboard(), SIGNAL(dataChanged()), this, SLOT(tarkistaLeikepoyta()));
    connect( ui->liitaNappi, &QPushButton::clicked, this, &NaytaliiteWg::leikepoydalta);

    setAcceptDrops(true);
    tarkistaLeikepoyta();
}

NaytaliiteWg::~NaytaliiteWg()
{
    delete ui;
}

void NaytaliiteWg::valitseTiedosto()
{
    QString polku = QFileDialog::getOpenFileName(this, tr("Valitse tosite"),QString(),tr("Pdf-tiedostot (*.pdf);;Kuvat (*.png *.jpg);;Csv-tiedosto (*.csv);;Kaikki tiedostot (*.*)"));
    if( !polku.isEmpty())
    {
        emit lisaaLiite( polku );
    }
}

void NaytaliiteWg::naytaPdf(const QByteArray &pdfdata)
{
    if( pdfdata.isEmpty())
    {
        setCurrentIndex(0);
    }
    else
    {
        setCurrentIndex(1);
        view->nayta(pdfdata);
    }
}

void NaytaliiteWg::leikepoydalta()
{
    if( qApp->clipboard()->mimeData()->formats().contains("image/jpg"))
        emit lisaaLiiteDatalla( qApp->clipboard()->mimeData()->data("image/jpg"), tr("liite.jpg") );
    else if( qApp->clipboard()->mimeData()->formats().contains("image/png"))
        emit lisaaLiiteDatalla( qApp->clipboard()->mimeData()->data("image/png"), tr("liite.png") );
}

void NaytaliiteWg::tarkistaLeikepoyta()
{

    QStringList formaatit = qApp->clipboard()->mimeData()->formats();

    ui->liitaNappi->setVisible( formaatit.contains("image/png") ||
                                formaatit.contains("image/jpg") );

}

void NaytaliiteWg::dragEnterEvent(QDragEnterEvent *event)
{
    if( event->mimeData()->hasUrls() )
    {
        for( const QUrl& url: event->mimeData()->urls())
        {
            if(url.isLocalFile())
                event->accept();
        }
    }


    if( event->mimeData()->formats().contains("image/jpg") ||
        event->mimeData()->formats().contains("image/png"))
        event->accept();
}

void NaytaliiteWg::dragMoveEvent(QDragMoveEvent *event)
{
    event->accept();
}

void NaytaliiteWg::dropEvent(QDropEvent *event)
{
    int lisatty = 0;
    // Liitetiedosto pudotettu
    if( event->mimeData()->hasUrls())
    {
        QList<QUrl> urlit = event->mimeData()->urls();
        foreach (QUrl url, urlit)
        {
            if( url.isLocalFile())
            {
                QString polku = url.path();

#ifdef Q_OS_WIN
                if( polku.startsWith(QChar('/')))
                    polku = polku.mid(1);
#endif
                emit lisaaLiite( polku );
                lisatty++;
            }
        }
    }
    if( !lisatty && event->mimeData()->formats().contains("image/jpg"))
        emit lisaaLiiteDatalla( event->mimeData()->data("image/jpg"), tr("liite.jpg") );
    else if(!lisatty && event->mimeData()->formats().contains("image/png"))
        emit lisaaLiiteDatalla( event->mimeData()->data("image/png"), tr("liite.png") );

    qDebug() << event->mimeData()->formats();
}


