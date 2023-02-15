#ifndef KUVALIITEWIDGET_H
#define KUVALIITEWIDGET_H

#include <QGraphicsView>
#include <QImage>
#include <QGraphicsScene>

#include <QLabel>
#include <QScrollArea>

class KuvaLiiteWidget : public QScrollArea
{
    Q_OBJECT
public:
    KuvaLiiteWidget(QWidget* parent = nullptr);

//    QGraphicsScene* scene_;
    QLabel* label;

    void nayta(const QImage& kuva);
};

#endif // KUVALIITEWIDGET_H
