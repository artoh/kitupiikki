#ifndef PVMRAPORTTIWIDGET_H
#define PVMRAPORTTIWIDGET_H

#include "raporttiwidget.h"

namespace Ui {
    class Paivakirja;
}

class PvmRaporttiWidget : public RaporttiWidget
{
    Q_OBJECT
public:
    PvmRaporttiWidget(const QString& tyyppi);
    virtual ~PvmRaporttiWidget();

    void esikatsele() override;

    QString tyyppi() const { return tyyppi_;}
protected:
    void lataa();
    void paivita();
    void piilotaTarpeettomat();

    virtual void tallenna();
    virtual void naytaRaportti();

private:
    void tiliListaSaapuu(QVariant *data);

protected:
    Ui::Paivakirja* ui;

private:
    QString tyyppi_;
};

#endif // PVMRAPORTTIWIDGET_H
