#ifndef LAATIJANPAIVAKIRJA_H
#define LAATIJANPAIVAKIRJA_H

#include "laatijanraportti.h"
#include "db/tilikausi.h"

class LaatijanPaivakirja : public LaatijanRaportti
{
    Q_OBJECT
public:
    LaatijanPaivakirja(RaportinLaatija* laatija, const RaporttiValinnat& valinnat);

    virtual void laadi();

private slots:
    void dataSaapuu(QVariant* data);

private:
    bool samatilikausi_ = true;
};

#endif // LAATIJANPAIVAKIRJA_H
