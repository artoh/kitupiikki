#include "pdfnaytin.h"

#include "db/kirjanpito.h"

#include <QPdfDocument>
#include <QPdfView>
#include <QBuffer>
#include <QPainter>
#include <QPrinter>
#include <QSettings>

#include <QImageReader>

namespace Naytin {

PdfNaytin::PdfNaytin(const QByteArray &ba, QObject *parent)
    : Naytin::AbstraktiNaytin{parent},
      doc_{new QPdfDocument(this)},
      buff_{new QBuffer(&data_, this)},
      view_{new QPdfView()}
{
    data_ = ba;
    buff_->open(QIODevice::ReadOnly);
    doc_->load(buff_);
    view_->setDocument(doc_);

    view_->setPageMode(QPdfView::PageMode::MultiPage);

    view_->setZoomMode(QPdfView::ZoomMode::FitToWidth);
    skaala_ = view_->zoomFactor();

    qDebug() << QImageReader::supportedImageFormats();
}

QWidget *PdfNaytin::widget()
{
    return view_;
}

QByteArray PdfNaytin::data() const
{
    return data_;
}

void PdfNaytin::paivita() const
{

}

void PdfNaytin::tulosta(QPrinter *printer) const
{
    QPainter painter(printer);

    QByteArray ba(data_);
    QBuffer buffer(&ba);
    buffer.open(QIODevice::ReadOnly);

    QImageReader reader(&buffer);

    const int pageCount = reader.imageCount();
    reader.setScaledSize(QSize(painter.window().width(), painter.window().height()));

    for(int i=0; i < pageCount; i++) {
        reader.jumpToImage(i);
        QImage image = reader.read();
        painter.drawImage(painter.window(), image);

        if( i < pageCount - 1)
            printer->newPage();
    }

/*
    QPainter painter(printer);
    const int pageCount = doc_->pageCount();

    for(int i=0; i < pageCount; i++) {
        QSizeF pdfSize = doc_->pagePointSize(i);
        qreal suhde = pdfSize.height() / pdfSize.width();
        QSize uusiKoko(painter.window().width(), suhde * painter.window().width());
        QImage kuva = doc_->render(i, uusiKoko);
        painter.drawImage(painter.window(), kuva);

        if( i < pageCount - 1)
            printer->newPage();

    }

*/
}

void PdfNaytin::zoomIn()
{
    view_->setZoomMode(QPdfView::ZoomMode::Custom);
    if( skaala_ < 0.5)
        skaala_ += 0.1;
    else
        skaala_ += 0.2;
    view_->setZoomFactor(skaala_);
}

void PdfNaytin::zoomOut()
{
    view_->setZoomMode(QPdfView::ZoomMode::Custom);
    if( skaala_ > 0.5)
        skaala_ -= 0.2;
    else if( skaala_ > 0.15)
        skaala_ -= 0.1;
    view_->setZoomFactor(skaala_);

}

void PdfNaytin::zoomFit()
{
    view_->setZoomMode(QPdfView::ZoomMode::FitToWidth);
    skaala_ = view_->zoomFactor();
}



} // namespace Naytin
