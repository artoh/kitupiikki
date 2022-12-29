#ifndef VEROMAARITYS_H
#define VEROMAARITYS_H

#include "tallentavamaarityswidget.h"

class VeroVarmenneTila;

namespace Ui {
    class VeroMaaritys;
}

class VeroMaaritys : public TallentavaMaaritysWidget
{
    Q_OBJECT
public:
    VeroMaaritys();
    ~VeroMaaritys();

    bool nollaa() override;

protected:
    void varmenneTiedotSaapuu(QVariant* data);

    void tilaPaivitetty();

    void haeViitteet();
    void viitteetSaapuu(QVariant* data);

    void lisaaVarmenne();
    void poistaVarmenne();

private:
    Ui::VeroMaaritys* ui;
    VeroVarmenneTila *tila;
};

#endif // VEROMAARITYS_H
