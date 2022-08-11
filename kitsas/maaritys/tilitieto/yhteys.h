#ifndef TILITIETO_YHTEYS_H
#define TILITIETO_YHTEYS_H


#include "pankki.h"
#include "yhdistettytili.h"
#include <QDateTime>
#include <QList>

namespace Tilitieto {

class PankitModel;

class Yhteys
{
public:
    Yhteys();
    Yhteys(const QVariantMap& map, PankitModel* pankitModel);

    Pankki pankki() const;
    QDateTime voimassa() const { return voimassa_;}

    int tileja() const;
    YhdistettyTili tili(int indeksi) const;

protected:    
    int pankkiId_;
    QDateTime voimassa_;

    QList<YhdistettyTili> tilit_;

    PankitModel* pankit_;


};

} // namespace Tilitieto

#endif // TILITIETO_YHTEYS_H
