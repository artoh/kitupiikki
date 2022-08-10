#ifndef TILITIETO_YHDISTETTYTILI_H
#define TILITIETO_YHDISTETTYTILI_H

#include <QDateTime>
#include <QVariantMap>
#include "laskutus/iban.h"

namespace Tilitieto {

class YhdistettyTili
{
public:
    YhdistettyTili();
    YhdistettyTili(QVariantMap map);

    Iban iban() const { return iban_; }
    QDateTime paivitetty() const { return paivitetty_; }    

protected:
    Iban iban_;    
    QDateTime paivitetty_;    
};

} // namespace Tilitieto

#endif // TILITIETO_YHDISTETTYTILI_H
