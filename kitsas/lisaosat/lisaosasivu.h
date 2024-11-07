#ifndef LISAOSASIVU_H
#define LISAOSASIVU_H

#include "qwebenginenewwindowrequest.h"
#include "qwebenginepermission.h"
#include <kitupiikkisivu.h>
#include <QObject>

class QWebEngineView;
class QListView;
class QLabel;
class QPushButton;
class LisaosaListModel;
class QAction;
class QWebEngineDownloadRequest;
class QSlider;
class QSplitter;
class LisaosaSivu : public KitupiikkiSivu
{
public:
    LisaosaSivu(QWidget* parent = nullptr);

    void siirrySivulle() override;

protected:
    enum Toiminto { NAYTA, AKTIVOI };

    void initSlider();
    void initUi();

    void valittu();
    void aktivoi();
    void yksityinen();
    void naytaLoki();

    void passivoi();

    void hae(const QString lisaosaId, Toiminto toiminto = NAYTA);

    void infoSaapuu(QVariant* data);
    void aktivointiInfoSaapuu(QVariant* data);

    void kuvakeVaihtui(const QIcon& icon);

    void listReseted();

    void openLinkInNewWindow(QWebEngineNewWindowRequest &request);
    void downloadRequested(QWebEngineDownloadRequest *download);
    void downloadFinished();
    void zoomChanged(int value);
    void permissionRequest(QWebEnginePermission permission);

protected:
    LisaosaListModel* listModel_;
    QSplitter* splitter_;
    QWebEngineView* webView_;
    QListView* listView_;
    QLabel* logoLabel_;
    QLabel* nameLabel_;
    QPushButton* aktivoiNappi_;
    QPushButton* passivoiNappi_;
    QPushButton* toiminnotNappi_;
    QAction* yksityinenAktio_;
    QAction* lokiAktio_;
    QLabel* zoomLabel_;
    QSlider* zoomSlider_;

    QPushButton* kotiNappi_;



    QString currentId_;


};

#endif // LISAOSASIVU_H
