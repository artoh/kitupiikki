#ifndef ERANSELVITYS_H
#define ERANSELVITYS_H

#include <QMainWindow>
#include <QWidget>
#include <QItemSelection>
#include <QDate>

#include "model/euro.h"

class EranSelvitysTiliModel;
class EranSelvitysEraModel;
class EranSelvitysViennit;
class EranSelvitysSortFilterProxyModel;
class QAction;

class QTableView;

class EranSelvitys : public QMainWindow
{
    Q_OBJECT
public:
    explicit EranSelvitys(QDate startDate, QDate endDate, QWidget *parent = nullptr);
    ~EranSelvitys();

signals:

protected:
    void initToolbar();
    void initActions();
    void initActionBar();

    void tiliValittu(const QItemSelection& selected);
    void eraValittu(const QItemSelection& selected);
    void naytaVienti();
    void naytaTaseErittely();

    void eratLadattu();

    Euro nollausSumma() const;
    void nollausTosite();
    void siirra();
    void uusiEra();
    void erittelemattomiin();
    void uudelleennimea();

    void paivita();
    void paivitaNapit();

    void tiliListaPaivitetty();
    void eraListaPaivitetty();
    void vientiListaPaivitetty();

    EranSelvitysTiliModel* tiliModel_;
    EranSelvitysEraModel *eraModel_;
    EranSelvitysViennit *viennit_;
    EranSelvitysSortFilterProxyModel *proxyModel_;

    QTableView* tiliView_;
    QTableView* eraView_;
    QTableView* viennitView_;

    int tili_ = -1;
    int eraId_ = 0;
    QList<int> valitutViennit_;
    QDate startDate_;
    QDate endDate_;

    QAction* avaaAktio_;
    QAction* uusiAktio_;
    QAction* siirtoAktio_;
    QAction* erittelematonAktio_;
    QAction* nollausAktio_;
    QAction* nimeaAktio_;

};

#endif // ERANSELVITYS_H
