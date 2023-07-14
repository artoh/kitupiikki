#ifndef LAATIJANKUUKAUSIRAPORTTI_H
#define LAATIJANKUUKAUSIRAPORTTI_H

#include "laatijanraportti.h"

class LaatijanKuukausiRaportti : public LaatijanRaportti
{
    Q_OBJECT
public:
    LaatijanKuukausiRaportti(RaportinLaatija* laatija, const RaporttiValinnat& valinnat);

    virtual void laadi() override;
    QString nimi() const override;

private:
    void dataSaapuu(QVariant* variant);
    bool yhteensaSarake_ = false;
    QString kieli_;    
};

#endif // LAATIJANKUUKAUSIRAPORTTI_H
