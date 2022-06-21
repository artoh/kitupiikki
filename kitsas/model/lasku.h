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
#include "laskutus/iban.h"
#include "laskutus/viitenumero.h"

class TositeRivit;


class Lasku : public KantaVariantti
{    
public:    

    enum Lahetys { TULOSTETTAVA, SAHKOPOSTI, VERKKOLASKU, PDF, EITULOSTETA, POSTITUS, TUOTULASKU };
    enum Maksutapa { LASKU, KATEINEN, ENNAKKOLASKU, SUORITEPERUSTE, KUUKAUSITTAINEN, KORTTIMAKSU };
    enum Valvonta { LASKUVALVONTA = 0, VAKIOVIITE = 2, ASIAKAS = 3, HUONEISTO = 4, VALVOMATON = 99 };
    enum Marginaali { KAYTETYT =91, TAIDE =  92, ANTIIKKI=93 };
    enum Rivityyppi { NETTORIVIT = 0, BRUTTORIVIT = 1, PITKATRIVIT = 2 } ;

    Lasku();
    Lasku(const QVariantMap& data);
    void kopioi(const Lasku& lasku);

    QString email() const { return str("email").trimmed();}
    void setEmail(const QString& email) { setStr("email",email);}

    QString kieli() const { return str("kieli");}
    void setKieli(const QString& kieli) { setStr("kieli", kieli);}

    int maksutapa() const { return luku("maksutapa");}
    void setMaksutapa(int maksutapa) { set("maksutapa", maksutapa);}

    int lahetystapa() const { return luku("laskutapa");}
    void setLahetystapa(int lahetystapa) { set("laskutapa", lahetystapa);}

    QString numero() const { return str("numero");}
    void setNumero(const QString& numero) { setStr("numero", numero);}

    QString osoite() const { return str("osoite");}
    void setOsoite(const QString& osoite) { setStr("osoite", osoite);}

    QString otsikko() const { return str("otsikko");}
    void setOtsikko(const QString& otsikko) { setStr("otsikko", otsikko);}

    QString saate() const { return str("saate");}
    void setSaate(const QString& saate) { setStr("saate", saate);}

    QString saateOtsikko() const { return str("saateotsikko");}
    void setSaateOtsikko(const QString& otsikko) { setStr("saateotsikko", otsikko);}

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

    Euro aiempiSaldo() const { return Euro::fromVariant(arvo("aiempisaldo"));}
    void setAiempiSaldo(const Euro saldo) { set("aiempisaldo", saldo);}

    QString asiakasViite() const { return str("asviite");}
    void setAsiakasViite(const QString& viite) { setStr("asviite", viite);}

    QString tilausNumero() const { return str("tilausnumero");}
    void setTilausNumero(const QString& tilausnumero) { set("tilausnumero", tilausnumero);}

    QString tilaaja() const { return str("tilaaja");}
    void setTilaaja(const QString& tilaaja) { setStr("tilaaja", tilaaja);}

    QDate tilausPvm() const { return pvm("tilauspvm");}
    void setTilausPvm(const QDate& tilauspvm) { set("tilauspvm", tilauspvm);}

    QString sopimusnumero() const { return str("sopimusnumero");}
    void setSopimusnumero(const QString& numero) { setStr("sopimusnumero",  numero);}

    QString lisatiedot() const { return str("lisatiedot");}
    void setLisatiedot(const QString& lisatiedot) { setStr("lisatiedot", lisatiedot);}

    QStringList erittely() const { return arvo("erittely").toStringList();}
    void setErittely(const QStringList& erittely) { if(erittely.isEmpty()) unset("erittely"); else set("erittely", erittely);}

    Euro summa() const { return euro("summa");}
    void setSumma(const Euro& summa) { setEuro("summa", summa);}

    Euro muistutusmaksu() const { return Euro::fromVariant(arvo("muistutusmaksu"));}
    void setMuistutusmaksu(const Euro maksu) { setStr("muistutusmaksu", maksu);}

    Euro korkoEuroa() const { return Euro::fromVariant(arvo("korko"));}
    void setKorko(double euroa) { set("korko", euroa);}

    QDate korkoAlkaa() const { return pvm("korkoalkaa");}
    void setKorkoAlkaa(const QDate& pvm) { set("korkoalkaa", pvm);}

    QDate korkoLoppuu() const { return pvm("korkoloppuu");}
    void setKorkoLoppuu(const QDate& pvm) { set("korkoloppuu", pvm);}

    QString iban() const { return str("iban");}

    void setToisto(const QDate& pvm, const int jaksoKuukautta, bool hinnastolla = true, const QDate& loppuu = QDate());
    void lopetaToisto();
    static QVariantMap toistoMap(const QDate& pvm, const int jaksoKuukautta, bool hinnastolla = true, const QDate& loppuu = QDate());
    // Päivä jolloin seuraava toistolasku lähtee
    QDate toistoPvm() const { return arvo("toisto").toMap().value("pvm").toDate();  }
    QDate toistoLoppuu() const { return arvo("toisto").toMap().value("loppuu").toDate();}
    int toistoJaksoPituus() const { return arvo("toisto").toMap().value("jaksonpituus").toInt();}
    bool toistoHinnastonMukaan() const { return arvo("toisto").toMap().value("hinnastolla").toBool();}

    QDate laskunpaiva() const { return pvm("pvm");}
    void setLaskunpaiva(const QDate& pvm) { set("pvm", pvm);}

    QDate erapvm() const { return pvm("erapvm");}
    void setErapaiva(const QDate& erapaiva) { set("erapvm", erapaiva);}

    ViiteNumero viite() const { return ViiteNumero(str("viite"));}
    void setViite(const ViiteNumero& viite) { setStr("viite", viite.viite());}

    QString myyja() const { return str("myyja");}
    void setMyyja(const QString& myyja) { setStr("myyja", myyja);}

    int huomautusAika() const { return luku("huomautusaika");}
    void setHuomautusAika(const int paivaa) { set("huomautusaika", paivaa);}

    int toistuvanErapaiva() const { return luku("toistuvanerapaiva");}
    void setToistuvanErapaiva(const int paiva) { set("toistuvanerapaiva", paiva);}

    QString virtuaaliviivakoodi(const Iban& iban, bool rf = false) const;
    QString QRkooditieto(const Iban& iban, const QString& nimi, bool rf=false) const;

    Rivityyppi riviTyyppi() const;
    void setRiviTyyppi(Rivityyppi tyyppi) { set("rivityyppi", tyyppi);}


    static QDate oikaiseErapaiva(QDate erapvm);

    QString tulkkaaMuuttujat(const QString& teksti) const;

};

#endif // LASKU_H
