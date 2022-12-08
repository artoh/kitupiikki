#ifndef MINAMAARITYS_H
#define MINAMAARITYS_H

#include "maarityswidget.h"

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

protected:
    void paivitaMoodi();
    void vaihdaSalasana();
    void tilausNappi();
    void laskutusTiedot();

private:
    Ui::MinaMaaritys *ui;
};

#endif // MINAMAARITYS_H
