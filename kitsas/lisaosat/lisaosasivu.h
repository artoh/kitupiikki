#ifndef LISAOSASIVU_H
#define LISAOSASIVU_H

#include <kitupiikkisivu.h>
#include <QObject>

class QWebEngineView;
class QListView;
class QLabel;
class QPushButton;
class LisaosaListModel;
class QAction;

class LisaosaSivu : public KitupiikkiSivu
{
public:
    LisaosaSivu(QWidget* parent = nullptr);

    void siirrySivulle() override;

protected:
    enum Toiminto { NAYTA, AKTIVOI };

    void initUi();

    void valittu(const QModelIndex& indeksi);
    void aktivoi();
    void yksityinen();
    void naytaLoki();

    void passivoi();

    void hae(const QString lisaosaId, Toiminto toiminto = NAYTA);

    void infoSaapuu(QVariant* data);
    void aktivointiInfoSaapuu(QVariant* data);

    void kuvakeVaihtui(const QIcon& icon);

    void listReseted();

protected:
    LisaosaListModel* listModel_;
    QWebEngineView* webView_;
    QListView* listView_;
    QLabel* logoLabel_;
    QLabel* nameLabel_;
    QPushButton* aktivoiNappi_;
    QPushButton* passivoiNappi_;
    QPushButton* toiminnotNappi_;
    QAction* yksityinenAktio_;
    QAction* lokiAktio_;

    QString currentId_;


};

#endif // LISAOSASIVU_H
