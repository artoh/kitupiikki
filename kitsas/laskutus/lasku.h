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

#include "model/tosite.h"
#include "db/kantavariantti.h"

class LaskuRivitModel;

class Lasku : public Tosite
{
    Q_OBJECT
public:    

    enum Lahetys { TULOSTETTAVA, SAHKOPOSTI, VERKKOLASKU, PDF, EITULOSTETA, POSTITUS, TUOTULASKU };
    enum Maksutapa { LASKU, KATEINEN, ENNAKKOLASKU, SUORITEPERUSTE, KUUKAUSITTAINEN };
    enum Valvonta { LASKUVALVONTA = 0, VAKIO = 2, ASIAKAS = 3, HUONEISTO = 4, VALVOMATON = 99 };

    Lasku(QObject* parent = nullptr);

    virtual void lataaData(QVariant* variant);
    LaskuRivitModel* rivit() { return rivit_;}

    QString email() const { return data_.str("email");}
    void setEmail(const QString& email) { data_.set("email",email);}

    QString kieli() const { return data_.str("kieli");}
    void setKieli(const QString& kieli) { data_.set("kieli", kieli);}

    int maksutapa() const { return data_.luku("maksutapa");}
    void setMaksutapa(int maksutapa) { data_.set("maksutapa", maksutapa);}

    int lahetystapa() const { return data_.luku("laskutapa");}
    void setLahetystapa(int lahetystapa) { data_.set("lahetystapa", lahetystapa);}

    qlonglong numero() const { return data_.arvo("numero").toLongLong();}
    void setNumero(qlonglong numero) { data_.set("numero", numero);}

    QString osoite() const { return data_.str("osoite");}
    void setOsoite(const QString& osoite) { data_.set("osoite", osoite);}

    QString otsikko() const { return data_.str("otsikko");}
    void setOtsikko(const QString& otsikko) { data_.set("otsikko", otsikko);}

    QString saate() const { return data_.str("saate");}
    void setSaate(const QString& saate) { data_.set("saate", saate);}

    int valvonta() const { return data_.luku("valvonta");}
    void setValvonta(int valvonta) { data_.set("valvonta", valvonta);}

    double viivastyskorko() const { return data_.dbl("viivkorko");}
    void setViivastyskorko(double viivastyskorko) { data_.set("viivkorko", viivastyskorko);}

    QDate toimituspvm() const { return data_.pvm("toimituspvm");}
    void setToimituspvm(const QDate& toimituspvm) { data_.set("toimituspvm", toimituspvm);}

    QDate jaksopvm() const { return data_.pvm("jaksopvm");}
    void setJaksopvm(const QDate& jaksopvm) { data_.set("jaksopvm", jaksopvm);}

    qlonglong alkuperaisNumero() const { return data_.luku("alkupNro");}
    void setAlkuperaisNumero(const qlonglong numero) { data_.set("alkupNro", numero);}

    QDate alkuperaisPvm() const { return data_.pvm("alkupPvm");}
    void setAlkuperaisPvm(const QDate& pvm) { data_.set("alkupPvm", pvm);}

    QVariantList aiemmat() const { return data_.arvo("aiemmat").toList(); }
    void setAiemmat(const QVariantList& aiemmat) { data_.set("aiemmat", aiemmat);}

    double aiempiSaldo() const { return data_.dbl("aiempisaldo");}
    void setAiempiSaldo(const double saldo) { data_.set("aiempisaldo", saldo);}

    QString asiakasViite() const { return data_.str("asviite");}
    void setAsiakasViite(const QString& viite) { data_.set("asviite", viite);}

    QString tilausNumero() const { return data_.str("tilausnumero");}
    void setTilausNumero(const QString& tilausnumero) { data_.set("tilausnumero", tilausnumero);}

    QString tilaaja() const { return data_.str("tilaaja");}
    void setTilaaja(const QString& tilaaja) { data_.set("tilaaja", tilaaja);}

    QDate tilausPvm() const { return data_.pvm("tilauspvm");}
    void setTilausPvm(const QString& tilauspvm) { data_.set("tilauspvm", tilauspvm);}

    QString sopimusnumero() const { return data_.str("sopimusnumero");}
    void setSopimusnumero(const QString& numero) { data_.set("sopimusnumero",  numero);}

    QString lisatiedot() const { return data_.str("lisatiedot");}
    void setLisatiedot(const QString& lisatiedot) { data_.set("lisatiedot", lisatiedot);}

    QStringList erittely() const { return data_.arvo("erittely").toStringList();}
    void setErittely(const QStringList& erittely) { data_.set("erittely", erittely);}

    double summa() const { return data_.dbl("summa");}
    void setSumma(const double summa) { data_.set("summa", summa);}

    double muistutusmaksu() const { return data_.dbl("muistutusmaksu");}
    void setMuistutusmaksu(const double maksu) { data_.set("muistutusmaksu", maksu);}

    double korkoEuroa() const { return data_.dbl("korko");}
    void setKorko(double euroa) { data_.set("korko", euroa);}

    QDate korkoAlkaa() const { return data_.pvm("korkoalkaa");}
    void setKorkoAlkaa(const QDate& pvm) { data_.set("korkoalkaa", pvm);}

    QDate korkoLoppuu() const { return data_.pvm("korkoloppuu");}
    void setKorkoLoppuu(const QDate& pvm) { data_.set("korkoloppuu", pvm);}

    QString iban() const { return data_.str("iban");}
    QString maventaId() const { return data_.str("maventaid");}

    // Päivä jolloin seuraava toistolasku lähtee
    QDate toistoPvm() const { return data_.pvm("toistopvm");}
    void setToistoPvm(const QDate& pvm) { data_.set("toistopvm", pvm);}

    QDate toistoLoppuu() const { return data_.pvm("toistoloppuu");}
    void setToistoLoppuu(const QDate& pvm) { data_.set("toistoloppuu", pvm);}

    int toistoJaksoPituus() const { return data_.luku("toistojaksopituus");}
    void setToistoJaksoPituus(const int kuukautta) { data_.set("toistojaksopituus", kuukautta);}

    bool toistoHinnastonMukaan() const { return data_.luku("toistohinnastonmukaan");}
    void setToistoHinnastonMukaan(bool onko) { if(onko) data_.set("toistohinnastonmukaan", 1); else data_.unset("toistohinnastonmukaan"); }

    static QDate oikaiseErapaiva(QDate erapvm);

protected:
    LaskuRivitModel* rivit_;
    KantaVariantti data_;

};

#endif // LASKU_H
