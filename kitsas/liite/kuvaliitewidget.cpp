#include "kuvaliitewidget.h"

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
    QImage skaalattu = kuva.scaledToWidth(width()-25);


    QPixmap pixmap = QPixmap::fromImage(skaalattu);
    label->setPixmap(pixmap);
    setWidget(label);
}
