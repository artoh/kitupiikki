#ifndef LAATIJANRAPORTTI_H
#define LAATIJANRAPORTTI_H

#include <QObject>

#include "../raportinkirjoittaja.h"
#include "../raporttivalinnat.h"

class RaportinLaatija;


class LaatijanRaportti : public QObject
{
    Q_OBJECT
public:
    LaatijanRaportti(RaportinLaatija* laatija, const RaporttiValinnat& valinnat);

    virtual void laadi() = 0;

    RaportinKirjoittaja raportinKirjoittaja() const { return rk;}
    RaporttiValinnat valinnat() const { return valinnat_;}

protected:
    void valmis();
    void tyhja();

    QString kaanna(const QString& teksti) const;
    QString kielikoodi() const { return kielikoodi_; }

protected:
    RaportinKirjoittaja rk;
private:
    RaporttiValinnat valinnat_;
    QString kielikoodi_;

signals:

};

#endif // LAATIJANRAPORTTI_H
