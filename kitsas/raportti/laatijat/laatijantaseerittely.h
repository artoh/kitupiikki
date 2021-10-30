#ifndef LAATIJANTASEERITTELY_H
#define LAATIJANTASEERITTELY_H

#include "laatijanraportti.h"

class LaatijanTaseErittely : public LaatijanRaportti
{
    Q_OBJECT
public:
    LaatijanTaseErittely(RaportinLaatija* laatija, const RaporttiValinnat& valinnat);

    virtual void laadi() override;
private:
    void dataSaapuu(QVariant* data);
    void lisaaTositeTunnus(RaporttiRivi* rivi, const QVariantMap& map);

    QDate mista_;
    QDate mihin_;
};

#endif // LAATIJANTASEERITTELY_H
