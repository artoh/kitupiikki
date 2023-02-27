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
#ifndef TULOMENORIVI_H
#define TULOMENORIVI_H

#include <QString>
#include <QVariantList>
#include <QDate>

#include "model/euro.h"

class Tosite;

/**
 * @brief Yksi Tulomenoapurilla kirjattava rivi
 */
class TulomenoRivi
{
public:
    TulomenoRivi(int tili=0);
    TulomenoRivi(const QVariantMap& data);

    int tilinumero() const { return tilinumero_; }
    int alvkoodi() const { return alvkoodi_; }
    double alvprosentti() const { return veroprosentti_; }
    bool alvvahennys() const { return alvvahennys_; }
    QString selite() const { return selite_;}

    int kohdennus() const { return kohdennus_; }
    QVariantList merkkaukset() const { return merkkaukset_; }

    QDate jaksoalkaa() const { return jaksoalkaa_;}
    QDate jaksopaattyy() const { return jaksoloppuu_;}

    int poistoaika() const { return poistoaika_;}

    void setTili(int numero) { tilinumero_=numero;}
    void setAlvkoodi(int koodi);
    void setAlvprosentti(double prosentti) { veroprosentti_ = prosentti;}
    void setAlvvahennys(bool vahennys);
    void setSelite(const QString& selite) { selite_ = selite;}
    void setKohdennus(int kohdennus) { kohdennus_ = kohdennus;}
    void setMerkkaukset(const QVariantList& merkkaukset) { merkkaukset_=merkkaukset;}
    void setJaksoalkaa(const QDate& pvm) { jaksoalkaa_ = pvm;}
    void setJaksopaattyy(const QDate& pvm) { jaksoloppuu_=pvm;}
    void setPoistoaika(int kk) { poistoaika_ = kk;}


    Euro brutto() const;
    Euro netto() const;

    bool nettoSyotetty() const { return netto_; }

    void setBrutto(Euro eurot);
    void setNetto(Euro eurot);
    void setNetonVero(Euro eurot);

    bool naytaBrutto() const;
    bool naytaNetto() const;
    bool naytaVahennysvalinta() const;

    QVariantList viennit(Tosite *tosite) const;


protected:
    int tilinumero_ = 0;

    Euro brutto_ = 0;
    Euro netto_ = 0;

    QString selite_;

    int alvkoodi_ = 0;
    double veroprosentti_ = 0;

    bool alvvahennys_ = false;

    int kohdennus_ = 0;
    QVariantList merkkaukset_;

    QDate jaksoalkaa_;
    QDate jaksoloppuu_;

    int vientiId_ = 0;
    int poistoaika_ = 0;

};

#endif // TULOMENORIVI_H
