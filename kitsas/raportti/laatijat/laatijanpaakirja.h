#ifndef LAATIJANPAAKIRJA_H
#define LAATIJANPAAKIRJA_H

#include "laatijanraportti.h"

class LaatijanPaakirja : public LaatijanRaportti
{
    Q_OBJECT
public:
    LaatijanPaakirja(RaportinLaatija* laatija, const RaporttiValinnat& valinnat);

    virtual void laadi();

private slots:
    void saldotSaapuu(QVariant* data);
    void viennitSaapuu(QVariant* data);

private:
    void kirjoitaDatasta();

private:

    QMap<QString,QList<QVariantMap>> data_;
    QMap<QString,Euro> saldot_;

    bool samatilikausi_ = true;
    int saapuneet_ = 0;

};

#endif // LAATIJANPAAKIRJA_H
