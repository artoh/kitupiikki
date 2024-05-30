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
#ifndef APURIRIVI_H
#define APURIRIVI_H

#include <QString>
#include <QVariantList>
#include <QDate>

#include "model/eramap.h"
#include "model/euro.h"
#include "model/tositevienti.h"

class Tosite;

/**
 * @brief Yksi apurilla kirjattava rivi
 */
class ApuriRivi
{
public:
    ApuriRivi(int tili=0);
    ApuriRivi(const TositeVienti& vienti, bool plusOnKredit = true);

    int tilinumero() const { return tilinumero_; }
    int alvkoodi() const;
    double alvprosentti() const { return veroprosentti_; }
    bool alvvahennys() const { return alvvahennys_; }
    QString selite() const { return selite_;}

    int kohdennus() const { return kohdennus_; }
    QVariantList merkkaukset() const { return merkkaukset_; }

    QDate jaksoalkaa() const { return jaksoalkaa_;}
    QDate jaksopaattyy() const { return jaksoloppuu_;}

    void setEra(const EraMap& era);
    EraMap era() const  { return era_;}
    int eraId() const { return era_.id();}

    int poistoaika() const;

    void setTili(int numero) { tilinumero_=numero;}
    void setAlvkoodi(int koodi);
    void setAlvprosentti(double prosentti);
    void setAlvvahennys(bool vahennys, int vahennysvientiId = 0);
    void setSelite(const QString& selite) { selite_ = selite;}
    void setKohdennus(int kohdennus) { kohdennus_ = kohdennus;}
    void setMerkkaukset(const QVariantList& merkkaukset) { merkkaukset_=merkkaukset;}
    void setJaksoalkaa(const QDate& pvm) { jaksoalkaa_ = pvm;}
    void setJaksopaattyy(const QDate& pvm) { jaksoloppuu_=pvm;}
    void setPoistoaika(int kk) { poistoaika_ = kk;}
    void setVientiId(int id) { vientiId_ = id;}


    Euro brutto() const;
    Euro netto() const;
    Euro naytettava() const;

    bool nettoSyotetty() const { return netto_; }

    void setBrutto(Euro eurot);
    void setNetto(Euro eurot);
    void setNetonVero(Euro eurot, int vientiId = 0);

    bool naytaBrutto() const;
    bool naytaNetto() const;
    bool naytaVahennysvalinta() const;

    QVariantList viennit(const TositeVienti::VientiTyyppi tyyppi, const bool plusOnKredit, const QString &otsikko = QString(),
                         const QVariantMap &kumppani = QVariantMap(), const QDate &pvm = QDate()) const;

    void setMaahantuonninAlv(int vientiId);
    void setVahentamaton(int vientiId);

    void vaihdaEtumerkki();

protected:
    int vientiId() const { return vientiId_;}

    int tilinumero_ = 0;

    Euro brutto_ = 0;
    Euro netto_ = 0;

    QString selite_;

    int alvkoodi_ = 0;
    double veroprosentti_ = 0;

    bool alvvahennys_ = true;

    int kohdennus_ = 0;
    QVariantList merkkaukset_;

    QDate jaksoalkaa_;
    QDate jaksoloppuu_;

    EraMap era_;

    int vientiId_ = 0;
    int veroVientiId_ = 0;
    int vahennysVientiId_ = 0;
    int maahantuontiVastaId_ = 0;
    int vahentamatonVientiId_ = 0;

    int poistoaika_ = 0;


};

#endif // APURIRIVI_H
