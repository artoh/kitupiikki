#ifndef LISAPALVELUTMAARITYS_H
#define LISAPALVELUTMAARITYS_H

#include "maarityswidget.h"

class LisaPalvelutMaaritys : public MaaritysWidget
{
    Q_OBJECT
public:
    LisaPalvelutMaaritys();



    bool naytetaankoTallennus() override { return false; }

protected:
    void setupUi();
};

#endif // LISAPALVELUTMAARITYS_H
