#ifndef LAATIJANTASETULOS_H
#define LAATIJANTASETULOS_H

#include "laatijanraportti.h"
#include "model/euro.h"

class LaatijanTaseTulos : public LaatijanRaportti
{
    Q_OBJECT
public:
    LaatijanTaseTulos(RaportinLaatija* laatija, const RaporttiValinnat& valinnat);

    virtual void laadi() override;
    QString nimi() const override;

private:
    void dataSaapuu(int sarake, QVariant* variant);
    void kirjoitaRaportti();
    void laadiTililista();
    void kirjoitaYlatunniste();
    QString sarakeTyyppiTeksti(const RaporttiValintaSarake::SarakeTyyppi& tyyppi) const;

private:
    QVariantMap kmap_;
    QString tyyppi_;

    bool erittelyt_ = false;

    int tilauslaskuri_ = 0;
    int sarakemaara_ = 0;

    QHash<int, QVector<Euro> > eurot_;   // tili -> sentit  (tot,tot,tot,bud,bud,bud)
    QMap<int, QHash<int, QVector<Euro> > > kohdennetut_;

    QStringList tilit_;

};

#endif // LAATIJANTASETULOS_H
