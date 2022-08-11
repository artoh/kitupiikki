#ifndef TILITIETO_TILITIETOPALVELU_H
#define TILITIETO_TILITIETOPALVELU_H



#include <QObject>
#include "yhteys.h"

#include "model/euro.h"

namespace Tilitieto{

class PankitModel;
class PankkiLokiModel;

class TilitietoPalvelu : public QObject
{
    Q_OBJECT
public:
    TilitietoPalvelu(QObject* parent = nullptr);

    int trialPeriod() const { return trialPeriod_;}
    Euro price() const { return price_;}

    PankitModel* pankit();

    void lisaaValtuutus(int pankkiId);
    void poistaValtuutus(int pankkiId);

    int yhteyksia() const;
    Yhteys yhteys(int indeksi);

    bool onkoValtuutettu(const QString& bic);

    void lataa();

    PankkiLokiModel* loki() { return loki_;}

protected:
    void lataaMap(const QVariant* data);
    void linkkiSaapuu(const QVariant* data);

signals:
    void ladattu();
    void vahvistaLinkilla(const QString& linkki, int pankkiId);

private:
    PankitModel* pankit_;
    QList<Yhteys> yhteydet_;

    int trialPeriod_;
    Euro price_;

    PankkiLokiModel* loki_;

};

} // namespace Tilitieto

#endif // TILITIETO_TILITIETOPALVELU_H
