#ifndef LAADITUNRAPORTINNAYTIN_H
#define LAADITUNRAPORTINNAYTIN_H

#include "raporttinaytin.h"
#include "raportti/raporttivalinnat.h"
#include "raportti/raportinlaatija.h"

namespace Naytin {


class LaaditunRaportinNaytin : public RaporttiNaytin
{
    Q_OBJECT
public:
    LaaditunRaportinNaytin(QWidget *parent, RaporttiValinnat valinnat = RaporttiValinnat());
    void paivitaRaportti();
    void paivitaRaportti(const RaporttiValinnat& valinnat);

    void virkista() override;
    bool voikoVirkistaa() const override { return true; }

    QString otsikko() const override { return otsikko_;}

public slots:
    void raporttiSaapuu(const RaportinKirjoittaja& kirjoittaja, const RaporttiValinnat& valinnat);

protected:
    RaporttiValinnat valinnat_;
    RaportinLaatija laatija_;
    QString otsikko_;
};


}
#endif // LAADITUNRAPORTINNAYTIN_H
