#ifndef PAAKIRJAPAIVAKIRJAKANTARAPORTTIWIDGET_H
#define PAAKIRJAPAIVAKIRJAKANTARAPORTTIWIDGET_H

#include "raporttiwidget.h"

namespace Ui {
    class Paivakirja;
}


class PaakirjaPaivakirjaKantaRaporttiWidget : public RaporttiWidget
{
    Q_OBJECT
public:
    PaakirjaPaivakirjaKantaRaporttiWidget();
    virtual ~PaakirjaPaivakirjaKantaRaporttiWidget();

public slots:
    virtual void esikatsele() override;

protected:
    virtual void tallennaValinnat() = 0;

    void paivitaKohdennukset();

    Ui::Paivakirja *ui;
};

#endif // PAAKIRJAPAIVAKIRJAKANTARAPORTTIWIDGET_H
