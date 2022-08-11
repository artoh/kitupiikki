#include "yhteys.h"
#include "pankitmodel.h"

namespace Tilitieto {

Yhteys::Yhteys()
{

}

Yhteys::Yhteys(const QVariantMap &map, PankitModel *pankitModel) :
    pankit_(pankitModel)
{    
    voimassa_ = map.value("valid").toDateTime();
    pankkiId_ = map.value("institution").toInt();

    QVariantList tilit = map.value("accounts").toList();
    for(QVariant tili : tilit) {
        QVariantMap tiliMap = tili.toMap();
        tilit_.append( YhdistettyTili(tiliMap) );
    }

}

Pankki Yhteys::pankki() const
{
    return pankit_->pankki(pankkiId_);
}

int Yhteys::tileja() const
{
    return tilit_.count();
}

YhdistettyTili Yhteys::tili(int indeksi) const
{
    return tilit_.at(indeksi);
}

} // namespace Tilitieto
