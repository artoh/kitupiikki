#ifndef PDFRENDERVIEW_H
#define PDFRENDERVIEW_H

#include <QGraphicsView>
#include <QPdfDocument>
#include <QGraphicsScene>
#include <QPdfView>

class LiitteetModel;

class PdfRenderView : public QGraphicsView
{
    Q_OBJECT
public:
    PdfRenderView(QWidget* parent = nullptr);
    QPdfDocument *document();    

    void setZoom(QPdfView::ZoomMode mode, qreal factor);
    void setDocument(QPdfDocument *document);

protected:
    void statusChanged(QPdfDocument::Status status);

    void dragEnterEvent(QDragEnterEvent *event) override;
    void dragMoveEvent(QDragMoveEvent *event) override;
    void dropEvent(QDropEvent *event) override;

    void resizeEvent(QResizeEvent* event) override;

protected:
    QPdfDocument* doc_ = nullptr;
    QPdfView::ZoomMode zoomMode_ = QPdfView::ZoomMode::FitToWidth;
    qreal zoomFactor_ = 1.00;


};

#endif // PDFRENDERVIEW_H
