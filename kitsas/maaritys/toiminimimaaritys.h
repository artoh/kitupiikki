#ifndef TOIMINIMIMAARITYS_H
#define TOIMINIMIMAARITYS_H

#include "maarityswidget.h"

namespace Ui {
    class ToiminimiMaaritys;
}

class QSortFilterProxyModel;

class ToiminimiMaaritys : public MaaritysWidget
{
    Q_OBJECT
public:
    ToiminimiMaaritys();
    ~ToiminimiMaaritys();

    bool nollaa() override;
    bool onkoMuokattu() override;
    bool tallenna() override;

protected:
    void lataa();
    void lataaLogo();
    void uusiToiminimi();
    void haeKaupunki();
    void poistaAputoiminimi();
    void vaihdaLogo();
    void poistaLogo();

private:
    Ui::ToiminimiMaaritys *ui;
    QSortFilterProxyModel *proxy;
};

#endif // TOIMINIMIMAARITYS_H
