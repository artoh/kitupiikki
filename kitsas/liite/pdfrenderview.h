#ifndef PDFRENDERVIEW_H
#define PDFRENDERVIEW_H

#include <QGraphicsView>
#include <QPdfDocument>
#include <QGraphicsScene>
#include <QPdfView>

class PdfRenderView : public QGraphicsView
{
    Q_OBJECT
public:
    PdfRenderView(QWidget* parent = nullptr);
    QPdfDocument *document();
    void setDocument(QPdfDocument* document);

    void setZoom(QPdfView::ZoomMode mode, qreal factor);

protected:
    void statusChanged(QPdfDocument::Status status);

protected:
    QPdfDocument* doc_ = nullptr;
    QPdfView::ZoomMode zoomMode_ = QPdfView::ZoomMode::FitToWidth;
    qreal zoomFactor_ = 1.00;

    void resizeEvent(QResizeEvent* event);

};

#endif // PDFRENDERVIEW_H
