#ifndef AVATTUPILVI_H
#define AVATTUPILVI_H

#include <QString>
#include <QVariantMap>

#include "palvelunkertoja.h"

class AvattuPilvi : public PalvelunKertoja
{
public:
    AvattuPilvi();
    AvattuPilvi(const QVariant& data);

    operator bool() const;

    int id() const { return id_; }
    int planId() const { return plan_id_;}
    bool vat() const { return vat_;}
    bool trial_period() const { return trial_period_; }

    QString url() const { return url_; }
    QString token() const { return token_;}

    qlonglong oikeudet() const { return oikeudet_; }

    /**
     * @brief Muodostaa oikeuksista bittikartan
     * @param Oikeudet listana ["Ts","Tl"] jne
     * @return YhteysModelin oikeuksista koostuva bittikartta
     */
    static qlonglong oikeudetListasta(const QVariantList& lista);

private:
    int id_ = 0;
    int plan_id_ = 0;
    bool vat_ = false;
    bool trial_period_ = false;

    qlonglong oikeudet_ = 0L;
    QString url_;
    QString token_;

    static std::map<QString,qlonglong> oikeustunnukset__;
};

#endif // AVATTUPILVI_H
