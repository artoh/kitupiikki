#ifndef LAATIJANMYYNTI_H
#define LAATIJANMYYNTI_H

#include "laatijanraportti.h"

class LaatijanMyynti : public LaatijanRaportti
{
    Q_OBJECT
public:
    LaatijanMyynti(RaportinLaatija *laatija, const RaporttiValinnat& valinnat);

    void laadi() override;

private:
    void dataSaapuu(QVariant* data);
};

#endif // LAATIJANMYYNTI_H
