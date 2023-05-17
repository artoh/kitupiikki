#ifndef MINAMAARITYS_H
#define MINAMAARITYS_H

#include "maarityswidget.h"
#include <QVariantMap>

namespace Ui {
class MinaMaaritys;
}

class MinaMaaritys : public MaaritysWidget
{
    Q_OBJECT
public:
    MinaMaaritys();
    ~MinaMaaritys();

    virtual bool nollaa();
    virtual bool onkoMuokattu();
    virtual bool tallenna();

protected:
    void paivitaMoodi();
    void vaihdaSalasana();
    void tilausNappi();
    void laskutusTiedot();

    void hae();
    void lueVastaus(QVariant* data);
    void tallennettu(QVariant* data);

    QVariantList paivalista() const;

private:
    Ui::MinaMaaritys *ui;
    QVariantMap minaMap_;
    QString keyid_;
};

#endif // MINAMAARITYS_H
