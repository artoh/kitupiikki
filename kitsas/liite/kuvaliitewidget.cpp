#include "kuvaliitewidget.h"

KuvaLiiteWidget::KuvaLiiteWidget(QWidget *parent) :
    QGraphicsView(parent),
    scene_{new QGraphicsScene(this)}
{
    setScene(scene_);
}

void KuvaLiiteWidget::nayta(const QImage &kuva)
{
    scene()->clear();
    QPixmap pixmap = QPixmap::fromImage(kuva);
    scene()->addPixmap(pixmap);
}
