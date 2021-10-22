#ifndef RAPORTINLAATIJA_H
#define RAPORTINLAATIJA_H


#include <QObject>
#include "raporttivalinnat.h"
#include "raportinkirjoittaja.h"

class LaatijanRaportti;
class RaportinLaatija : public QObject
{
    Q_OBJECT
public:
    RaportinLaatija(QObject *parent = nullptr);

    void laadi(const RaporttiValinnat& valinnat);

    void valmis(LaatijanRaportti* raportti);
    void tyhja(LaatijanRaportti* raportti);

signals:
    void raporttiValmis(const RaportinKirjoittaja& kirjoittaja, const RaporttiValinnat& valinnat);
    void tyhjaRaportti(const RaporttiValinnat& valinnat);

};

#endif // RAPORTINLAATIJA_H
