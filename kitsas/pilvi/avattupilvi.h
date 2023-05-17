#ifndef AVATTUPILVI_H
#define AVATTUPILVI_H

#include <QString>
#include <QVariantMap>

#include "palvelunkertoja.h"
#include "pilviextra.h"
#include "aloitussivu/aloitusinfot.h"

class AvattuPilvi : public PalvelunKertoja, public AloitusInfot
{
public:
    enum { PROPILVI = 200};

    AvattuPilvi();
    AvattuPilvi(const QVariant& data);

    operator bool() const;

    int id() const { return id_; }
    QString nimi() const { return name_;}
    QString ytunnus() const { return businessid_;}
    bool kokeilu() const { return trial_;}
    bool aktiivinen() const { return active_;}
    int planId() const { return plan_id_;}
    bool vat() const { return vat_;}
    bool trial_period() const { return trial_period_; }

    QString url() const { return url_; }
    QString token() const { return token_;}
    QString alias() const { return alias_;}

    qlonglong oikeudet() const { return oikeudet_; }

    bool alustettu() const { return alustettu_;}

    void asetaNimi(const QString& nimi);
    void asetaYTunnus(const QString& ytunnus);
    void asetaAlias(const QString& alias);

    PilviExtra extra(int id) const;
    QList<int> extrat() const;

    void asetaNotifikaatiot(const QVariantList& lista);

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
    bool trial_ = false;
    bool active_ = false;

    qlonglong oikeudet_ = 0L;
    QString url_;
    QString token_;
    bool alustettu_ = false;
    QString name_;
    QString businessid_;
    QString alias_;

    QMap<int,PilviExtra> extrat_;

    static std::map<QString,qlonglong> oikeustunnukset__;
};

#endif // AVATTUPILVI_H
