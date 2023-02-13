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

NaytaLiiteWidget::NaytaLiiteWidget(QWidget *parent)
    : QWidget{parent},
      uusiLiiteWidget_{ new UusiLiiteWidget(this)}
{
    setup();

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
            kuvaView_->nayta(kuva);
            pino_->setCurrentIndex(KUVA);
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


void NaytaLiiteWidget::setup()
{
    tabBar_ = new QTabBar;
    tabBar_->setShape(QTabBar::TriangularWest);

    tabBar_->addTab("Tiedosto 1");
    tabBar_->addTab("Tiedosto 2");

    pino_ = new QStackedWidget;
    pino_->addWidget( uusiLiiteWidget_ );

    QLabel* latausLabel = new QLabel(tr("Ladataan..."));
    latausLabel->setAlignment(Qt::AlignCenter);
    pino_->addWidget(latausLabel);

    QLabel* tyhjaLabel = new QLabel();
    pino_->addWidget(tyhjaLabel);

    pdfView_ = new QPdfView();
    pino_->addWidget(pdfView_);

    kuvaView_ = new KuvaLiiteWidget(this);
    pino_->addWidget(kuvaView_);

    textView_ = new QTextEdit(this);
    textView_->setReadOnly(true);
    pino_->addWidget(textView_);


    QHBoxLayout *leiska = new QHBoxLayout;
    leiska->addWidget(tabBar_);
    leiska->addWidget(pino_);

    setLayout(leiska);

    pdfView_->setPageMode(QPdfView::PageMode::MultiPage);
    pdfView_->setZoomMode(QPdfView::ZoomMode::FitToWidth);
}

