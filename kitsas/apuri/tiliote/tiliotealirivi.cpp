#include "tiliotealirivi.h"
#include "apuri/tiliote/tiliotekirjausrivi.h"

TilioteAliRivi::TilioteAliRivi()
{
    alvvahennys_ = true;
}

TilioteAliRivi::TilioteAliRivi(const QVariantMap &data)
    : TulomenoRivi(data)
{
    era_ = EraMap(data.value("era").toMap());
}

void TilioteAliRivi::setEra(EraMap era)
{
    era_ = era;
}

QVariantList TilioteAliRivi::viennit(const int tyyppi, const QString &otsikko, const QVariantMap &kumppani, const QDate &pvm) const
{
    if( tyyppi == TilioteKirjausRivi::OSTO ||
        tyyppi == TilioteKirjausRivi::MYYNTI) {
        return TulomenoRivi::viennit(tyyppi, otsikko, kumppani, pvm);
    }

    TositeVienti vienti;

    vienti.setTili( tilinumero() );
    vienti.setPvm( pvm );
    vienti.setSelite( otsikko );

    vienti.setKumppani( kumppani );
    vienti.setEra( era() );
    vienti.setKredit( brutto() );

    if( vientiId() > 0) {
        vienti.setId( vientiId() );
    }


    vienti.setTyyppi( tyyppi == TilioteKirjausRivi::SUORITUS ?
        TilioteKirjausRivi::SIIRTO + TositeVienti::KIRJAUS :
        tyyppi + TositeVienti::KIRJAUS);

    return QVariantList() << vienti;
}
