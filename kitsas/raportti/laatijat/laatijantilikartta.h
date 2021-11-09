#ifndef LAATIJANTILIKARTTA_H
#define LAATIJANTILIKARTTA_H

#include "laatijanraportti.h"

class LaatijanTilikartta : public LaatijanRaportti
{
    Q_OBJECT
public:
    LaatijanTilikartta(RaportinLaatija* laatija, const RaporttiValinnat& valinnat);

    virtual void laadi() override;
private:
    virtual void saldotSaapuu(QVariant* data);

};

#endif // LAATIJANTILIKARTTA_H
