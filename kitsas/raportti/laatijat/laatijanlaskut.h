#ifndef LAATIJANLASKUT_H
#define LAATIJANLASKUT_H

#include "laatijanraportti.h"

class LaatijanLaskut : public LaatijanRaportti
{
    Q_OBJECT
public:
    LaatijanLaskut(RaportinLaatija *laatija, const RaporttiValinnat& valinnat);

    void laadi() override;
private:
    void dataSaapuu(QVariant* data);
    bool lajitteluVertailu(const QVariant& eka, const QVariant& toka);

    QString lajittelu_;
    QDate saldopvm_;
    bool myyntilaskut_;
};

#endif // LAATIJANLASKUT_H
