#ifndef NAYTALIITEWIDGET_H
#define NAYTALIITEWIDGET_H


#include <QWidget>
#include <QByteArray>
#include <QBuffer>
#include <QPdfView>

class LiitteetModel;

class QStackedWidget;
class UusiLiiteWidget;
class QTabBar;
class QBuffer;
class QTextEdit;
class QTextBrowser;
class PdfRenderView;

class KuvaLiiteWidget;

class NaytaLiiteWidget : public QWidget
{
    Q_OBJECT
public:
    enum { UUSI, LATAA, TYHJA, PDF, KUVA, TEKSTI };
    explicit NaytaLiiteWidget(QWidget *parent = nullptr);

    void setModel(LiitteetModel* model);

    void naytaPohjat(bool naytetaanko);

    void tulosta();
    void tallenna();
    void avaa();

signals:
    void lataaPohja(int tositeId);

protected:
    void tulostaPdf();
    void setup();
    void alustaAktionit();

    void vaihdaValittu(int indeksi);
    void naytaPdf();
    void naytaSisalto();
    void paivitaTabit();

    void dragEnterEvent(QDragEnterEvent *event) override;
    void dragMoveEvent(QDragMoveEvent *event) override;
    void dropEvent(QDropEvent *event) override;

    void scaleZoom(qreal scale);
    void fitZoom(QPdfView::ZoomMode mode);

    void refreshZoom();
    void contextMenuEvent(QContextMenuEvent *event) override;


    UusiLiiteWidget* uusiLiiteWidget_;
    QTabBar* tabBar_;
    QStackedWidget* pino_;

    QPdfView* pdfView_ = nullptr;
    PdfRenderView* pdfRender_ = nullptr;
    KuvaLiiteWidget* kuvaView_;
    QTextBrowser* textView_;

    LiitteetModel* model_ = nullptr;

    QPdfView::ZoomMode zoomMode_ = QPdfView::ZoomMode::FitToWidth;
    qreal zoomFactor_ = 1.00;


    QAction* zoomAktio_;
    QAction* zoomFitAktio_;
    QAction* zoomInAktio_;
    QAction* zoomOutAktio_;
    QAction* tulostaAktio_;
    QAction* tallennaAktio_;
    QAction* avaaAktio_;


};

#endif // NAYTALIITEWIDGET_H
