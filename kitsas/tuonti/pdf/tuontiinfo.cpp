#include "tuontiinfo.h"

#include "db/kirjanpito.h"
#include "db/asetusmodel.h"
#include "model/toiminimimodel.h"
#include "db/tilimodel.h"

namespace Tuonti {

TuontiInfo::TuontiInfo()
{


}

void TuontiInfo::paivita()
{
    // Alustetaan tähän kaikki tuonnissa tarvittava !

    omatNimet_.clear();

    ToiminimiModel* nimet = kp()->toiminimet();
    for(int i=0; i < nimet->rowCount(); i++) {
        QString nimi = nimet->tieto(ToiminimiModel::Nimi, i);
        if( nimi.length() > 5)
            nimi = nimi.left(nimi.length()-3);
        omatNimet_.append(nimi);
    }

    yTunnus_ = kp()->asetukset()->ytunnus();

    TiliModel* tilit = kp()->tilit();

    omatIbanit_.clear();

    for(int i=0; i < tilit->rowCount(); i++) {
        Tili* tili = tilit->tiliPIndeksilla(i);
        if( tili->iban().isValid() )
            omatIbanit_.append(tili->iban().valeitta());
    }
}

} // namespace Tuonti
