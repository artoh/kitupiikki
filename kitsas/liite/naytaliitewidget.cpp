#include "naytaliitewidget.h"
#include "liite/uusiliitewidget.h"
#include "liite/liitteetmodel.h"

#include "kuvaliitewidget.h"
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
#include <QPrintDialog>
#include <QPrinter>
#include <QSettings>
#include <QFileDialog>
#include <QFile>
#include <QMessageBox>
#include <QDesktopServices>

#include <QMenu>
#include <QTimer>

#include "db/kirjanpito.h"
#include "pdfrenderview.h"


NaytaLiiteWidget::NaytaLiiteWidget(QWidget *parent)
    : QWidget{parent},
      uusiLiiteWidget_{ new UusiLiiteWidget(this)}
{
    setup();
    alustaAktionit();

    connect( uusiLiiteWidget_, &UusiLiiteWidget::lataaPohja, this, &NaytaLiiteWidget::lataaPohja);

    setAcceptDrops(true);
}

void NaytaLiiteWidget::setModel(LiitteetModel *model)
{
    model_ = model;    
    uusiLiiteWidget_->setModel(model);

    if(pdfView_)
        pdfView_->setDocument( model_->pdfDocument() );
    else if( pdfRender_)
        pdfRender_->setDocument( model_->pdfDocument() );

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
    if( pino_->currentIndex() == PDF) {
        tulostaPdf();
    } else if(pino_->currentIndex() == KUVA)
        kuvaView_->tulosta(kp()->printer());
    else if( pino_->currentIndex() == TEKSTI) {
        QPrintDialog dlg(kp()->printer(), this);
        if( dlg.exec()) {
            textView_->print(kp()->printer());
        }
    }
}

void NaytaLiiteWidget::tallenna()
{
    const QModelIndex& nykyinen = model_->index( model_->naytettavaIndeksi(),0 );
    if( !nykyinen.isValid()) return;

    const QDir polku(kp()->settings()->value(kp()->asetukset()->uid() + "/raporttipolku", QDir::homePath()).toString());
    const QString& tiedostonnimi = nykyinen.data(LiitteetModel::NimiRooli).toString();
    const QString tiedosto = QFileDialog::getSaveFileName(this, tr("Tallenna liite"), polku.absoluteFilePath(tiedostonnimi));
    if( !tiedosto.isEmpty()) {
        kp()->settings()->setValue( kp()->asetukset()->uid() + "/raporttipolku", polku.absolutePath() );

        QFile file(tiedosto);
        if( !file.open( QIODevice::WriteOnly))
        {
            QMessageBox::critical(this, tr("Tiedoston tallentaminen"),
                                  tr("Tiedostoon %1 kirjoittaminen epäonnistui.").arg(tiedosto));
            return;
        }
        file.write( nykyinen.data(LiitteetModel::SisaltoRooli).toByteArray() );
    }

}

void NaytaLiiteWidget::avaa()
{
    const QModelIndex& nykyinen = model_->index( model_->naytettavaIndeksi(),0 );
    if( !nykyinen.isValid()) return;

    QByteArray data = nykyinen.data(LiitteetModel::SisaltoRooli).toByteArray();
    QString nimi = nykyinen.data(LiitteetModel::NimiRooli).toByteArray();
    QString paate = nimi.mid(nimi.lastIndexOf("."));

    QString tiedostonnimi = kp()->tilapainen( QString("liite-XXXX").append(paate) );

    QFile tiedosto( tiedostonnimi);
    tiedosto.open( QIODevice::WriteOnly);
    tiedosto.write( data);
    tiedosto.close();

    if( !QDesktopServices::openUrl( QUrl::fromLocalFile(tiedosto.fileName()) ))
        QMessageBox::critical(this, tr("Tiedoston avaaminen"), tr("%1-tiedostoja näyttävän ohjelman käynnistäminen ei onnistunut").arg( paate ) );


}

void NaytaLiiteWidget::tulostaPdf()
{
    QPdfDocument* doc = pdfView_ ? pdfView_->document() : pdfRender_->document();
    QPrinter* printer = kp()->printer();

    QPrintDialog dlg(printer, this);
    dlg.setMinMax(1, doc->pageCount() + 1);


    if( dlg.exec() ) {
        QPainter painter(printer);
        QSizeF kohde(painter.window().width(), painter.window().height());

        int sivulta = 0;
        int sivulle = doc->pageCount();

        if( dlg.printRange() == QPrintDialog::PrintRange::PageRange ) {
            sivulta = dlg.fromPage() - 1;
            sivulle = dlg.toPage() - 1;
        }

        for(int i=sivulta; i < sivulle; i++) {

            if( i > sivulta)
                printer->newPage();

            QSizeF koko = doc->pagePointSize(i);
            QPdfDocumentRenderOptions options;
            if( koko.width() > koko.height())
                options.setRotation(QPdfDocumentRenderOptions::Rotation::Clockwise90);
            QImage image = doc->render(i, kohde.toSize(), options);
            painter.drawImage(0,0,image);
        }
    }

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
            refreshZoom();
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

void NaytaLiiteWidget::scaleZoom(qreal scale)
{
    zoomFactor_ *= scale;
    zoomMode_ = QPdfView::ZoomMode::Custom;
    refreshZoom();
}

void NaytaLiiteWidget::fitZoom(QPdfView::ZoomMode mode)
{
    zoomMode_ = mode;
    refreshZoom();
}

void NaytaLiiteWidget::refreshZoom()
{
    if( pdfView_) {
        pdfView_->setZoomFactor(zoomFactor_);
        pdfView_->setZoomMode(zoomMode_);
    } else if( pdfRender_ ) {
        pdfRender_->setZoom(zoomMode_, zoomFactor_);
    }
    if( pino_->currentIndex() == KUVA) {
        kuvaView_->setZoom(zoomMode_, zoomFactor_);
    }
}

void NaytaLiiteWidget::contextMenuEvent(QContextMenuEvent *event)
{
    if( pino_->currentIndex() < PDF) return;

    QMenu valikko(this);
    if( pino_->currentIndex() != TEKSTI )
    {
        valikko.addAction(zoomAktio_);
        valikko.addAction(zoomFitAktio_);
        valikko.addAction(zoomInAktio_);
        valikko.addAction(zoomOutAktio_);
        valikko.addSeparator();
    }
    valikko.addAction(avaaAktio_);
    valikko.addAction(tulostaAktio_);
    valikko.addAction(tallennaAktio_);

    valikko.exec( event->globalPos() );
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

    if( kp()->settings()->value("PikaPdf").toBool()) {
        pdfView_ = new QPdfView();
        pino_->addWidget(pdfView_);
        pdfView_->setPageMode(QPdfView::PageMode::MultiPage);
        pdfView_->setZoomMode(QPdfView::ZoomMode::FitToWidth);
    } else {
        pdfRender_ = new PdfRenderView();
        pino_->addWidget( pdfRender_ );
    }

    kuvaView_ = new KuvaLiiteWidget(this);
    pino_->addWidget(kuvaView_);

    textView_ = new QTextBrowser(this);
//    textView_->setReadOnly(true);
    pino_->addWidget(textView_);


    QHBoxLayout *leiska = new QHBoxLayout;
    leiska->addWidget(tabBar_);
    leiska->addWidget(pino_);


    setLayout(leiska);
}

void NaytaLiiteWidget::alustaAktionit()
{
    zoomAktio_ = new QAction( QIcon(":/pic/zoom-fit-width.png"), tr("Sovita leveyteen"),this);
    connect( zoomAktio_, &QAction::triggered, this, [this] {this->fitZoom(QPdfView::ZoomMode::FitToWidth);});

    zoomFitAktio_ = new QAction( QIcon(":/pic/zoom-fit.png"), tr("Sovita sivu"),this);
    connect( zoomFitAktio_, &QAction::triggered, this, [this] {this->fitZoom(QPdfView::ZoomMode::FitInView);});

    zoomInAktio_ = new QAction( QIcon(":/pic/zoom-in.png"), tr("Suurenna"), this);
    zoomInAktio_->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_Plus));
    connect( zoomInAktio_, &QAction::triggered, this, [this] {this->scaleZoom(1.2);});

    zoomOutAktio_ = new QAction( QIcon(":/pic/zoom-out.png"), tr("Pienennä"), this);
    zoomOutAktio_->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_Minus));
    connect( zoomOutAktio_, &QAction::triggered, this, [this] {this->scaleZoom( 1.0 / 1.2);});

    tulostaAktio_ = new QAction( QIcon(":/pic/tulosta.png"), tr("Tulosta"), this);
    connect( tulostaAktio_, &QAction::triggered, this, &NaytaLiiteWidget::tulosta);

    tallennaAktio_ = new QAction( QIcon(":/pic/tiedostoon.png"), tr("Tallenna"), this);
    connect( tallennaAktio_, &QAction::triggered, this, &NaytaLiiteWidget::tallenna);

    avaaAktio_ = new QAction( QIcon(":/pic/pdf.png"), tr("Avaa"), this);
    connect( avaaAktio_, &QAction::triggered, this, &NaytaLiiteWidget::avaa);

}

