#ifndef KUVALIITEWIDGET_H
#define KUVALIITEWIDGET_H

#include <QGraphicsView>
#include <QImage>
#include <QGraphicsScene>

class KuvaLiiteWidget : public QGraphicsView
{
    Q_OBJECT
public:
    KuvaLiiteWidget(QWidget* parent = nullptr);

    QGraphicsScene* scene_;
    void nayta(const QImage& kuva);
};

#endif // KUVALIITEWIDGET_H
