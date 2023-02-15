#ifndef TUONTI_TUONTIINFO_H
#define TUONTI_TUONTIINFO_H

#include "tuontiapuinfo.h"

namespace Tuonti {

class TuontiInfo : public TuontiApuInfo
{
public:
    TuontiInfo();

    void paivita();
};

} // namespace Tuonti

#endif // TUONTI_TUONTIINFO_H
