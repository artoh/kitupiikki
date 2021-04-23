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
#ifndef TOSITERIVI_H
#define TOSITERIVI_H

#include "db/kantavariantti.h"

class TositeRivi : public KantaVariantti
{
public:   

    TositeRivi(const QVariantMap& data = QVariantMap());

    int tuote() const { return luku("tuote");}
    void setTuote(const int tuoteId) { set("tuote", tuoteId);}

    double myyntiKpl() const { return dbl("myyntikpl");}
    void setMyyntiKpl(const double kpl) { set("myyntikpl", kpl);}

    double ostoKpl() const { return dbl("ostokpl");}
    void setOstoKpl(const double kpl) { set("ostokpl", kpl);}

    double aNetto() const { return dbl("ahinta");}
    double aBrutto() const;
    void setANetto(const double hinta) { set("ahinta", hinta);}
    void setABrutto(const double hinta);

    QString nimike() const { return str("nimike");}
    void setNimike(const QString& nimike) { setStr("nimike", nimike);}

    QString kuvaus() const { return str("kuvaus");}
    void setKuvaus(const QString& kuvaus) { setStr("kuvaus", kuvaus);}

    QString yksikko() const { return str("yksikko");}
    void setYksikko(const QString& yksikko) { setStr("yksikko", yksikko);}

    int tili() const { return luku("tili");}
    void setTili(const int tili) { setInt("tili", tili);}

    int kohdennus() const { return luku("kohdennus");}
    void setKohdennus(const int kohdennus) { setInt("kohdennus", kohdennus);}

    QVariantList merkkaukset() const { return arvo("merkkaukset").toList(); }
    void setMerkkaukset(const QVariantList& lista) { set("merkkaukset", lista);}

    int alvkoodi() const { return luku("alvkoodi");}
    void setAlvKoodi(const int alvkoodi) { set("alvkoodi", alvkoodi);}

    double alvProsentti() const { return dbl("alvprosentti");}
    void setAlvProsentti(const double prosentti) { set("alvprosentti", prosentti);}

    double aleProsentti() const { return dbl("aleprosentti");}
    void setAleProsentti(const double prosentti) { set("aleprosentti", prosentti);}
    double laskettuAleProsentti() const;

    /** Euroalennus nettona */
    double euroAlennus() const { return dbl("euroalennus");}
    void setEuroAlennus(const double euro) { set("euroalennus", euro ); }
    double bruttoEuroAlennus() const;
    void setBruttoEuroAlennus(const double euro);
    double laskennallinenEuroAlennus() const;
    double laskennallinenBruttoEuroAlennus() const;

    int alennusSyy() const { return luku("alennussyy");}
    void setAlennusSyy(const int syy) { set("alennussyy", syy);}

    QString toimitettuKpl() const { return str("toimitettu");}
    void setToimitettuKpl(const QString& kpl) { setStr("toimitettu", kpl);}

    QString jalkitoimitusKpl() const { return str("jalkitoimitus");}
    void setJalkitoimitusKpl(const QString& kpl) { setStr("jalkitoimitus", kpl);}

    QString laskutetaanKpl() const { return str("laskutetaan");}
    void setLaskutetaanKpl(const QString& kpl) { setStr("laskutetaan", kpl);}

    QString unKoodi() const { return str("unkoodi");}
    void setUNkoodi(const QString& koodi) { setStr("unkoodi", koodi);}

    Euro bruttoYhteensa() const;
    void setBruttoYhteensa(const Euro& euro);
    void setNettoYhteensa(const double netto);

    double nettoYhteensa() const;

    QString lisatiedot() const { return str("lisatiedot");}
    void setLisatiedot(const QString& tiedot) { setStr("lisatiedot", tiedot);}

    int ennakkoEra() const { return luku("ennakkoera");}
    void setEnnakkoEra(const int era) { set("ennakkoera", era); }

};

#endif // TOSITERIVI_H
