#ifndef LAATIJANTOSITELUETTELO_H
#define LAATIJANTOSITELUETTELO_H

#include "laatijanraportti.h"

class LaatijanTositeLuettelo : public LaatijanRaportti
{
    Q_OBJECT
public:
    LaatijanTositeLuettelo(RaportinLaatija* laatija, const RaporttiValinnat& valinnat);

    virtual void laadi();
private:
    void dataSaapuu(QVariant* data);

};

#endif // LAATIJANTOSITELUETTELO_H
