#include "kuvaliitewidget.h"

#include <QPrintDialog>
#include <QPainter>
#include <QPrinter>

KuvaLiiteWidget::KuvaLiiteWidget(QWidget *parent) :
    QScrollArea(parent),
    label(new QLabel)
{

    setAlignment(Qt::AlignCenter);
}

void KuvaLiiteWidget::nayta(const QImage &kuva)
{    
    kuva_ = kuva;
    paivita();
}

void KuvaLiiteWidget::tulosta(QPrinter *printer)
{
    QPrintDialog dlg(printer, this);
    if( dlg.exec()) {
        QPainter painter(printer);

        QSize size = kuva_.size();
        size.scale(painter.viewport().size(), Qt::KeepAspectRatio);
        QRect rect(painter.viewport().x(), painter.viewport().y(),
                   size.width(), size.height());
        painter.drawImage(rect, kuva_);
    }

}

void KuvaLiiteWidget::paivita()
{
    QImage image;
    if( zoomMode_ == QPdfView::ZoomMode::FitInView) {
        QSize koko(width() - 20, height() - 20);
        image = kuva_.scaled(koko, Qt::KeepAspectRatio, Qt::SmoothTransformation);
    } else if( zoomMode_ == QPdfView::ZoomMode::FitToWidth) {
        image = kuva_.scaledToWidth(width() - 20, Qt::SmoothTransformation);
    } else {
        QSize koko = kuva_.size() * zoomFactor_;
        image = kuva_.scaled(koko, Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
    }
    QPixmap pixmap = QPixmap::fromImage(image);
    setAlignment(Qt::AlignCenter);

    delete takeWidget();
    label = new QLabel();
    label->setPixmap(pixmap);

    setWidget(label);
}

void KuvaLiiteWidget::setZoom(QPdfView::ZoomMode mode, qreal factor)
{
    zoomMode_ = mode;
    zoomFactor_ = factor;
    paivita();
}

