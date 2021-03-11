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
#ifndef LASKU_H
#define LASKU_H

#include "db/kantavariantti.h"
#include <QDate>

class TositeRivit;


class Lasku : public KantaVariantti
{    
public:    

    enum Lahetys { TULOSTETTAVA, SAHKOPOSTI, VERKKOLASKU, PDF, EITULOSTETA, POSTITUS, TUOTULASKU };
    enum Maksutapa { LASKU, KATEINEN, ENNAKKOLASKU, SUORITEPERUSTE, KUUKAUSITTAINEN };
    enum Valvonta { LASKUVALVONTA = 0, VAKIOVIITE = 2, ASIAKAS = 3, HUONEISTO = 4, VALVOMATON = 99 };
    enum Marginaali { KAYTETYT =91, TAIDE =  92, ANTIIKKI=93 };


    Lasku();
    Lasku(const QVariantMap& data);

    QString email() const { return str("email");}
    void setEmail(const QString& email) { set("email",email);}

    QString kieli() const { return str("kieli");}
    void setKieli(const QString& kieli) { set("kieli", kieli);}

    int maksutapa() const { return luku("maksutapa");}
    void setMaksutapa(int maksutapa) { set("maksutapa", maksutapa);}

    int lahetystapa() const { return luku("laskutapa");}
    void setLahetystapa(int lahetystapa) { set("lahetystapa", lahetystapa);}

    QString numero() const { return str("numero");}
    void setNumero(const QString& numero) { set("numero", numero);}

    QString osoite() const { return str("osoite");}
    void setOsoite(const QString& osoite) { set("osoite", osoite);}

    QString otsikko() const { return str("otsikko");}
    void setOtsikko(const QString& otsikko) { set("otsikko", otsikko);}

    QString saate() const { return str("saate");}
    void setSaate(const QString& saate) { set("saate", saate);}

    int valvonta() const { return luku("valvonta");}
    void setValvonta(int valvonta) { set("valvonta", valvonta);}

    double viivastyskorko() const { return dbl("viivkorko");}
    void setViivastyskorko(double viivastyskorko) { set("viivkorko", viivastyskorko);}

    QDate toimituspvm() const { return pvm("toimituspvm");}
    void setToimituspvm(const QDate& toimituspvm) { set("toimituspvm", toimituspvm);}

    QDate jaksopvm() const { return pvm("jaksopvm");}
    void setJaksopvm(const QDate& jaksopvm) { set("jaksopvm", jaksopvm);}

    qlonglong alkuperaisNumero() const { return luku("alkupNro");}
    void setAlkuperaisNumero(const qlonglong numero) { set("alkupNro", numero);}

    QDate alkuperaisPvm() const { return pvm("alkupPvm");}
    void setAlkuperaisPvm(const QDate& pvm) {set("alkupPvm", pvm);}

    QVariantList aiemmat() const { return arvo("aiemmat").toList(); }
    void setAiemmat(const QVariantList& aiemmat) { set("aiemmat", aiemmat);}

    double aiempiSaldo() const { return dbl("aiempisaldo");}
    void setAiempiSaldo(const double saldo) { set("aiempisaldo", saldo);}

    QString asiakasViite() const { return str("asviite");}
    void setAsiakasViite(const QString& viite) { set("asviite", viite);}

    QString tilausNumero() const { return str("tilausnumero");}
    void setTilausNumero(const QString& tilausnumero) { set("tilausnumero", tilausnumero);}

    QString tilaaja() const { return str("tilaaja");}
    void setTilaaja(const QString& tilaaja) { set("tilaaja", tilaaja);}

    QDate tilausPvm() const { return pvm("tilauspvm");}
    void setTilausPvm(const QString& tilauspvm) { set("tilauspvm", tilauspvm);}

    QString sopimusnumero() const { return str("sopimusnumero");}
    void setSopimusnumero(const QString& numero) { set("sopimusnumero",  numero);}

    QString lisatiedot() const { return str("lisatiedot");}
    void setLisatiedot(const QString& lisatiedot) { set("lisatiedot", lisatiedot);}

    QStringList erittely() const { return arvo("erittely").toStringList();}
    void setErittely(const QStringList& erittely) { set("erittely", erittely);}

    double summa() const { return dbl("summa");}
    void setSumma(const double summa) { set("summa", summa);}

    double muistutusmaksu() const { return dbl("muistutusmaksu");}
    void setMuistutusmaksu(const double maksu) { set("muistutusmaksu", maksu);}

    double korkoEuroa() const { return dbl("korko");}
    void setKorko(double euroa) { set("korko", euroa);}

    QDate korkoAlkaa() const { return pvm("korkoalkaa");}
    void setKorkoAlkaa(const QDate& pvm) { set("korkoalkaa", pvm);}

    QDate korkoLoppuu() const { return pvm("korkoloppuu");}
    void setKorkoLoppuu(const QDate& pvm) { set("korkoloppuu", pvm);}

    QString iban() const { return str("iban");}
    QString maventaId() const { return str("maventaid");}

    // Päivä jolloin seuraava toistolasku lähtee
    QDate toistoPvm() const { return pvm("toistopvm");}
    void setToistoPvm(const QDate& pvm) { set("toistopvm", pvm);}

    QDate toistoLoppuu() const { return pvm("toistoloppuu");}
    void setToistoLoppuu(const QDate& pvm) { set("toistoloppuu", pvm);}

    int toistoJaksoPituus() const { return luku("toistojaksopituus");}
    void setToistoJaksoPituus(const int kuukautta) { set("toistojaksopituus", kuukautta);}

    bool toistoHinnastonMukaan() const { return luku("toistohinnastonmukaan");}
    void setToistoHinnastonMukaan(bool onko) { if(onko) set("toistohinnastonmukaan", 1); else unset("toistohinnastonmukaan"); }

    QDate laskunpaiva() const { return pvm("pvm");}
    void setLaskunpaiva(const QDate& pvm) { set("pvm", pvm);}

    QDate erapvm() const { return pvm("erapvm");}
    void setErapaiva(const QDate& erapaiva) { set("erapvm", erapaiva);}

    QString viite() const { return str("viite");}
    void setViite(const QString& viite) { set("viite", viite);}


    static QDate oikaiseErapaiva(QDate erapvm);

};

#endif // LASKU_H
