/*
   Copyright (C) 2019 Arto Hyvättinen

   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program. If not, see <http://www.gnu.org/licenses/>.
*/
#ifndef TILIOTEKIRJAUSRIVI_H
#define TILIOTEKIRJAUSRIVI_H

#include "tilioterivi.h"
#include "model/tositevienti.h"

class TilioteKirjausRivi : public TilioteRivi
{
public:        
    TilioteKirjausRivi();
    TilioteKirjausRivi(const QVariantList& data, TilioteModel* model);
    TilioteKirjausRivi(const QVariantMap& tuonti, TilioteModel* model);
    TilioteKirjausRivi(const QDate& pvm, TilioteModel* model);
    TilioteKirjausRivi(const QList<TositeVienti>& viennit, TilioteModel* model);

    QVariant riviData(int sarake, int role) const;
    bool setRiviData(int sarake, const QVariant& value);
    Qt::ItemFlags riviFlags(int sarake) const;

    void peita(bool onko) { peitetty_ = onko;}
    bool peitetty() const { return peitetty_;}
    bool tuotu() const { return tuotu_;}

    QList<TositeVienti> viennit() const;
    TositeVienti pankkivienti() const;
    QVariantList tallennettavat() const;

    void asetaPankkitili(int tili);
    void asetaViennit(const QList<TositeVienti> &viennit);

    QString pseudoarkistotunnus() const;
    void alkuperaistositeSaapuu(QVariant* data);

protected:    
    void paivitaTyyppi();
    void paivitaErikoisrivit();

    bool peitetty_ = false;
    bool tuotu_ = false;

    QList<TositeVienti> viennit_;

    int haettuEra_ = 0;
    QList<TositeVienti> alkuperaisViennit_;
};

#endif // TILIOTEKIRJAUSRIVI_H
