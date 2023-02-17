#include "naytaliitewidget.h"
#include "liite/uusiliitewidget.h"
#include "liite/liitteetmodel.h"

#include "kuvaliitewidget.h"
#include "pdfliiteview.h"
#include "tuonti/csvtuonti.h"

#include <QStackedWidget>
#include <QTabBar>
#include <QHBoxLayout>
#include <QLabel>

#include <QPdfDocument>
#include <QPdfView>
#include <QBuffer>
#include <QTextEdit>
#include <QTextBrowser>

#include <QMimeData>
#include <QDragEnterEvent>
#include <QDragMoveEvent>
#include <QDropEvent>

#include "db/kirjanpito.h"


NaytaLiiteWidget::NaytaLiiteWidget(QWidget *parent)
    : QWidget{parent},
      uusiLiiteWidget_{ new UusiLiiteWidget(this)}
{
    setup();

    connect( uusiLiiteWidget_, &UusiLiiteWidget::lataaPohja, this, &NaytaLiiteWidget::lataaPohja);

    setAcceptDrops(true);
}

void NaytaLiiteWidget::setModel(LiitteetModel *model)
{
    model_ = model;    
    uusiLiiteWidget_->setModel(model);
    pdfView_->setDocument( model_->pdfDocument() );

    connect( model, &LiitteetModel::valittuVaihtui, this, &NaytaLiiteWidget::vaihdaValittu);
    connect( model, &LiitteetModel::naytaPdf, this, &NaytaLiiteWidget::naytaPdf);
    connect( model, &LiitteetModel::naytaSisalto, this, &NaytaLiiteWidget::naytaSisalto);

    connect( model, &LiitteetModel::modelReset, this, &NaytaLiiteWidget::paivitaTabit);
    connect( model, &LiitteetModel::rowsInserted, this, &NaytaLiiteWidget::paivitaTabit);
    connect( model, &LiitteetModel::rowsRemoved, this, &NaytaLiiteWidget::paivitaTabit);

    connect( tabBar_, &QTabBar::tabBarClicked, model_, &LiitteetModel::nayta);
}

void NaytaLiiteWidget::naytaPohjat(bool naytetaanko)
{
    uusiLiiteWidget_->naytaPohja(naytetaanko);
}

void NaytaLiiteWidget::tulosta()
{
    if( pino_->currentIndex() == PDF)
        pdfView_->tulosta( kp()->printer() );
    else if(pino_->currentIndex() == KUVA)
        kuvaView_->tulosta(kp()->printer());
}

void NaytaLiiteWidget::vaihdaValittu(int indeksi)
{
    if(indeksi < 0) {
        pino_->setCurrentIndex(UUSI);
    } else {
        pino_->setCurrentIndex(LATAA);
        tabBar_->setCurrentIndex(indeksi);
    }
}

void NaytaLiiteWidget::naytaPdf()
{
    pino_->setCurrentIndex( PDF );
}

void NaytaLiiteWidget::naytaSisalto()
{
    QByteArray* data = model_->sisalto();
    if( data ) {
        QImage kuva = QImage::fromData(*data);
        if(!kuva.isNull()) {
            pino_->setCurrentIndex(KUVA);
            kuvaView_->nayta(kuva);
        } else {
            const QString teksti = Tuonti::CsvTuonti::haistettuKoodattu(*data);
            textView_->setPlainText(teksti);
            pino_->setCurrentIndex(TEKSTI);
        }
    }
}

void NaytaLiiteWidget::paivitaTabit()
{
    if( model_->rowCount() < 2) {
        tabBar_->hide();
    } else {
        while( tabBar_->count() < model_->rowCount())
            tabBar_->addTab("");
        while( tabBar_->count() > model_->rowCount())
            tabBar_->removeTab(0);
        for(int i=0; i < model_->rowCount(); i++) {
            tabBar_->setTabText(i, model_->index(i,0).data(Qt::DisplayRole).toString());
        }
        tabBar_->show();
    }
}

void NaytaLiiteWidget::dragEnterEvent(QDragEnterEvent *event)
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

void NaytaLiiteWidget::dragMoveEvent(QDragMoveEvent *event)
{
    event->accept();
}

void NaytaLiiteWidget::dropEvent(QDropEvent *event)
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
                model_->lisaaHetiTiedosto(polku);
                lisatty++;
            }
        }
    }
    if( !lisatty && event->mimeData()->formats().contains("image/jpg"))
        model_->lisaaHeti(event->mimeData()->data("image/jpg"), tr("liite.jpg") );
    else if(!lisatty && event->mimeData()->formats().contains("image/png"))
        model_->lisaaHeti(event->mimeData()->data("image/png"), tr("liite.png") );

}


void NaytaLiiteWidget::setup()
{
    tabBar_ = new QTabBar;
    tabBar_->setShape(QTabBar::TriangularWest);
    tabBar_->setExpanding(false);
    tabBar_->setUsesScrollButtons(true);
    tabBar_->setSizePolicy(QSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred));

    pino_ = new QStackedWidget;
    pino_->addWidget( uusiLiiteWidget_ );

    QLabel* latausLabel = new QLabel(tr("Ladataan..."));
    latausLabel->setAlignment(Qt::AlignCenter);
    pino_->addWidget(latausLabel);

    QLabel* tyhjaLabel = new QLabel();
    pino_->addWidget(tyhjaLabel);

    pdfView_ = new PdfLiiteView();
    pino_->addWidget(pdfView_);

    kuvaView_ = new KuvaLiiteWidget(this);
    pino_->addWidget(kuvaView_);

    textView_ = new QTextBrowser(this);
//    textView_->setReadOnly(true);
    pino_->addWidget(textView_);


    QHBoxLayout *leiska = new QHBoxLayout;
    leiska->addWidget(tabBar_);
    leiska->addWidget(pino_);


    setLayout(leiska);

    pdfView_->setPageMode(QPdfView::PageMode::MultiPage);
    pdfView_->setZoomMode(QPdfView::ZoomMode::FitToWidth);
}

