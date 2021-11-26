#ifndef LAATIJANALV_H
#define LAATIJANALV_H

#include "laatijanraportti.h"

class AlvLaskelma;

class LaatijanAlv : public LaatijanRaportti
{
    Q_OBJECT
public:
    LaatijanAlv(RaportinLaatija *laatija, const RaporttiValinnat& valinnat);

    void laadi() override;
private:
    void laadittu(RaportinKirjoittaja kirjoittaja);

    AlvLaskelma* laskelma = nullptr;
};

#endif // LAATIJANALV_H
