#include "kuvaliitewidget.h"

#include <QPrintDialog>
#include <QPainter>
#include <QPrinter>

KuvaLiiteWidget::KuvaLiiteWidget(QWidget *parent) :
    QScrollArea(parent),
    label(new QLabel)
//    QGraphicsView(parent),
//    scene_{new QGraphicsScene(this)}
{
//    setScene(scene_);

    setAlignment(Qt::AlignCenter);
}

void KuvaLiiteWidget::nayta(const QImage &kuva)
{    
    qDebug() << " kuva " << kuva.width() << " näkymä " << viewport()->sizeHint().width() << " widget " << width();
    kuva_ = kuva;
    QImage skaalattu = kuva.scaledToWidth(width()-25);


    QPixmap pixmap = QPixmap::fromImage(skaalattu);
    label->setPixmap(pixmap);
    setWidget(label);
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

