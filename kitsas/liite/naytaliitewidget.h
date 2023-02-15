#ifndef NAYTALIITEWIDGET_H
#define NAYTALIITEWIDGET_H


#include <QWidget>
#include <QByteArray>
#include <QBuffer>

class LiitteetModel;

class QStackedWidget;
class UusiLiiteWidget;
class QTabBar;
class QPdfView;
class QBuffer;
class QTextEdit;
class QTextBrowser;

class KuvaLiiteWidget;

class NaytaLiiteWidget : public QWidget
{
    Q_OBJECT
public:
    enum { UUSI, LATAA, TYHJA, PDF, KUVA, TEKSTI };
    explicit NaytaLiiteWidget(QWidget *parent = nullptr);

    void setModel(LiitteetModel* model);

    void naytaPohjat(bool naytetaanko);

signals:
    void lataaPohja(int tositeId);

protected:
    void setup();

    void vaihdaValittu(int indeksi);
    void naytaPdf();
    void naytaSisalto();
    void paivitaTabit();

    void dragEnterEvent(QDragEnterEvent *event);
    void dragMoveEvent(QDragMoveEvent *event);
    void dropEvent(QDropEvent *event);

    UusiLiiteWidget* uusiLiiteWidget_;
    QTabBar* tabBar_;
    QStackedWidget* pino_;

    QPdfView* pdfView_;
    KuvaLiiteWidget* kuvaView_;
    QTextBrowser* textView_;

    LiitteetModel* model_ = nullptr;



};

#endif // NAYTALIITEWIDGET_H
