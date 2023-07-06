#include "pdfrenderview.h"
#include <QGraphicsPixmapItem>

PdfRenderView::PdfRenderView(QWidget *parent) :
    QGraphicsView(parent)
{
    setScene(new QGraphicsScene(this));
}

QPdfDocument *PdfRenderView::document()
{
    return doc_;
}

void PdfRenderView::setDocument(QPdfDocument *document)
{
    doc_ = document;
    connect( document, &QPdfDocument::statusChanged, this, &PdfRenderView::statusChanged);
}

void PdfRenderView::setZoom(QPdfView::ZoomMode mode, qreal factor)
{
    zoomMode_ = mode;
    zoomFactor_ = factor;
    if( doc_) statusChanged( doc_->status());
}

void PdfRenderView::statusChanged(QPdfDocument::Status status)
{
    if( status == QPdfDocument::Status::Unloading) return;

    scene()->setBackgroundBrush(QBrush(Qt::gray));
    scene()->clear();
    if( !doc_ ) return;
    QPdfDocumentRenderOptions options;
    options.setRenderFlags( QPdfDocumentRenderOptions::RenderFlag::Annotations);

    if( status == QPdfDocument::Status::Ready) {
        double ypos = 0;

        for(int page = 0; page < doc_->pageCount(); page++) {
            QSizeF pointSize = doc_->pagePointSize(page);

            QSize size;

            if( zoomMode_ == QPdfView::ZoomMode::FitInView ) {
                size = pointSize.toSize().scaled( viewport()->width() - 20, viewport()->height() - 20, Qt::KeepAspectRatio );
            } else if( zoomMode_ == QPdfView::ZoomMode::FitToWidth) {
                const int areaWidth = viewport()->width() - 20;
                const bool myVisible = isVisible();
                size = QSize(  areaWidth, pointSize.height() / pointSize.width() * areaWidth );
            } else {
                size = pointSize.toSize() * zoomFactor_;
            }

            QImage image = doc_->render(page, size,options);
            QPixmap pixmap = QPixmap::fromImage(image, Qt::DiffuseAlphaDither);

            scene()->addRect(0, ypos, pixmap.width(), pixmap.height(), QPen(QBrush(Qt::black), 2.00), QBrush(Qt::white) );
            QGraphicsPixmapItem *item = scene()->addPixmap(pixmap);
            item->setY( ypos);

            ypos += pixmap.height() + 10.0;
        }
    }
}

void PdfRenderView::resizeEvent(QResizeEvent * /*event*/)
{
    if( doc_ && zoomMode_ != QPdfView::ZoomMode::Custom) statusChanged( doc_->status());
}


