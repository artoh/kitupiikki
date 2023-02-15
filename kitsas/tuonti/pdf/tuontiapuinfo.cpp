#include "tuontiapuinfo.h"

namespace Tuonti {

TuontiApuInfo::TuontiApuInfo()
{

}

bool TuontiApuInfo::omaNimi(const QString &nimi) const
{
    for(const auto& omanimi : omatNimet_) {
        if( nimi.contains(omanimi, Qt::CaseInsensitive)) return true;
    }
    return false;
}

bool TuontiApuInfo::omaYtunnus(const QString &ytunnus) const
{
    return yTunnus_ == ytunnus;
}

bool TuontiApuInfo::omaIban(const QString &iban) const
{
    return omatIbanit_.contains(iban, Qt::CaseInsensitive);
}

} // namespace Tuonti
