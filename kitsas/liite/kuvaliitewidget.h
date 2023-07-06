#ifndef KUVALIITEWIDGET_H
#define KUVALIITEWIDGET_H

#include <QGraphicsView>
#include <QImage>
#include <QGraphicsScene>

#include <QLabel>
#include <QScrollArea>
#include <QPdfView>

class QPrinter;

class KuvaLiiteWidget : public QScrollArea
{
    Q_OBJECT
public:
    KuvaLiiteWidget(QWidget* parent = nullptr);

//    QGraphicsScene* scene_;
    QImage kuva_;
    QLabel* label;

    void nayta(const QImage& kuva);
    void tulosta(QPrinter* printer);

    void paivita();
    void setZoom(QPdfView::ZoomMode mode, qreal factor);

    QPdfView::ZoomMode zoomMode_ = QPdfView::ZoomMode::FitToWidth;
    qreal zoomFactor_ = 1.00;

};

#endif // KUVALIITEWIDGET_H
