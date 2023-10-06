#ifndef HUBTOIMISTOSIVU_H
#define HUBTOIMISTOSIVU_H

#include <kitupiikkisivu.h>

class QWebEngineView;

class HubToimistoSivu : public KitupiikkiSivu
{
    Q_OBJECT
public:
    enum Jarjestelma { ADMIN, MAJAVA};

    HubToimistoSivu(QWidget *parent, Jarjestelma jarjestelma = ADMIN);

    void siirrySivulle() override;

protected:
    QWebEngineView* view_;
    Jarjestelma jarjestelma_;
};

#endif // HUBTOIMISTOSIVU_H
