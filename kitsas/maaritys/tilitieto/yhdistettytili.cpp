#include "yhdistettytili.h"

namespace Tilitieto {

YhdistettyTili::YhdistettyTili()
{

}

YhdistettyTili::YhdistettyTili(QVariantMap map)
{
    iban_ = Iban(map.value("iban").toString());    
    paivitetty_ = map.value("fetched").toDateTime();
}

} // namespace Tilitieto
