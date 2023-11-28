#ifndef TILIOTEALIRIVI_H
#define TILIOTEALIRIVI_H

#include "apuri/tulomenorivi.h"
#include "model/eramap.h"

class TilioteAliRivi : public TulomenoRivi
{
public:
    TilioteAliRivi();
    TilioteAliRivi(const QVariantMap& data);

    void setEra(EraMap era);
    EraMap era() const { return era_;}
    int eraId() const { return era_.id();}

    QVariantList viennit(const int tyyppi, const QString &otsikko = QString(),
                         const QVariantMap &kumppani = QVariantMap(), const QDate &pvm = QDate()) const;

protected:
    EraMap era_;
};

#endif // TILIOTEALIRIVI_H
