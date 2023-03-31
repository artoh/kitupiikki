#ifndef LISAPALVELUTMAARITYS_H
#define LISAPALVELUTMAARITYS_H

#include "../maarityswidget.h"

class QVBoxLayout;
class QLabel;

class LisaPalvelutMaaritys : public MaaritysWidget
{
    Q_OBJECT
public:
    LisaPalvelutMaaritys();

    bool naytetaankoTallennus() override { return false; }

protected:
    void setupUi();

    void updateExtras();
    void showExtras(QVariant* data);

    QVBoxLayout* extraLayout;    
};

#endif // LISAPALVELUTMAARITYS_H
