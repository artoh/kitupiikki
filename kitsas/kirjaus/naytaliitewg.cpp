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

#include <QGraphicsPixmapItem>

#include "naytaliitewg.h"
#include "db/kirjanpito.h"

#include "mallipohjamodel.h"

#include "naytin/naytinview.h"


NaytaliiteWg::NaytaliiteWg(QWidget *parent)
    : QStackedWidget(parent)
{
    QWidget *paasivu = new QWidget(this);
    ui = new Ui::TositeWg();
    ui->setupUi(paasivu);
    ui->pohjaGroup->setVisible(false);

    addWidget(paasivu);

    view = new NaytinView(this);
    connect( view, &NaytinView::tiedostoPudotettu, this, &NaytaliiteWg::lisaaLiite);
    addWidget(view);

    QLabel* llabel = new QLabel("Ladataan...");
    llabel->setAlignment(Qt::AlignCenter);
    addWidget(llabel);

    connect(ui->valitseTiedostoNappi, SIGNAL(clicked(bool)), this, SLOT(valitseTiedosto()));
    connect( qApp->clipboard(), SIGNAL(dataChanged()), this, SLOT(tarkistaLeikepoyta()));
    connect( ui->liitaNappi, &QPushButton::clicked, this, &NaytaliiteWg::leikepoydalta);

    ui->malliView->setModel(MallipohjaModel::instanssi());
    connect(ui->malliView, &QListView::clicked, this, [this] (const QModelIndex& index)
        {emit this->lataaPohja(index.data(Qt::UserRole).toInt());});
    connect( MallipohjaModel::instanssi(), &MallipohjaModel::modelReset, this, &NaytaliiteWg::pohjatSaapui );

    setAcceptDrops(true);
    tarkistaLeikepoyta();
    pohjatSaapui();
}

NaytaliiteWg::~NaytaliiteWg()
{
    delete ui;
}

void NaytaliiteWg::valitseTiedosto()
{
    QString polku = QFileDialog::getOpenFileName(this, tr("Valitse tosite"),QString(),tr("Pdf-tiedostot (*.pdf);;Kuvat (*.png *.jpg);;Csv-tiedosto (*.csv);;Kaikki tiedostot (*)"));
    if( !polku.isEmpty())
    {
        emit lisaaLiite( polku );
    }
}

void NaytaliiteWg::naytaPdf(const QByteArray &pdfdata, bool salliPudotus)
{
    if( pdfdata == "*LADATAAN*") {
        setCurrentIndex(2);
    } else if( pdfdata.isEmpty())
    {
        setCurrentIndex(0);
    }
    else
    {
        view->nayta(pdfdata, salliPudotus);
        setCurrentIndex(1);
    }
}

void NaytaliiteWg::leikepoydalta()
{
    if( qApp->clipboard()->mimeData()->formats().contains("image/jpg"))
        emit lisaaLiiteDatalla( qApp->clipboard()->mimeData()->data("image/jpg"), tr("liite.jpg") );
    else if( qApp->clipboard()->mimeData()->formats().contains("image/png"))
        emit lisaaLiiteDatalla( qApp->clipboard()->mimeData()->data("image/png"), tr("liite.png") );
}

void NaytaliiteWg::naytaPohjat(bool nayta)
{
    pohjatNakyvilla_ = nayta;
    if(currentIndex() == 0)
        ui->pohjaGroup->setVisible(pohjatNakyvilla_  && ui->malliView->model()->rowCount());
}

void NaytaliiteWg::pohjatSaapui()
{
    if(currentIndex() == 0)
        ui->pohjaGroup->setVisible(pohjatNakyvilla_  && ui->malliView->model()->rowCount());
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

}


