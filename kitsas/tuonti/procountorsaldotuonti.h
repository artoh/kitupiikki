#ifndef PROCOUNTORSALDOTUONTI_H
#define PROCOUNTORSALDOTUONTI_H

#include <QVariantMap>
#include "tilimuuntomodel.h"
#include "db/tilikausi.h"

class ProcountorSaldoTuonti
{
protected:
    ProcountorSaldoTuonti();

public:
    static QVariantMap tuo(const QByteArray& data);

    static QDate alustaMuunto(const QByteArray& data, TiliMuuntoModel& muunto, const Tilikausi& kausi = Tilikausi());

    static QDate kkPaivaksi(const QString teksti);

protected:
    static QRegularExpression tiliRE__;
};

#endif // PROCOUNTORSALDOTUONTI_H
