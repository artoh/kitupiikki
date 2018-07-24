/*
   Copyright (C) 2017 Arto Hyvättinen

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


/**
 * @dir db
 * @brief Tietokantaan (kirjanpidon hakemistoon) liittyvät luokat
 *
 */

#ifndef KIRJANPITO_H
#define KIRJANPITO_H

#include <QObject>
#include <QMap>
#include <QDir>
#include <QSqlDatabase>
#include <QDate>
#include <QTemporaryDir>
#include <QImage>

#include "tili.h"
#include "tilikausi.h"

#include "tositelajimodel.h"
#include "asetusmodel.h"
#include "tilimodel.h"
#include "tilikausimodel.h"
#include "tositemodel.h"
#include "kohdennusmodel.h"
#include "verotyyppimodel.h"
#include "tilityyppimodel.h"

#include "laskutus/tuotemodel.h"

class QPrinter;


/**
 * @brief Kirjanpidon käsittely
 *
 * Ainokainen, jonka kautta käsitellään tietokantaa
 *
 *
 */
class Kirjanpito : public QObject
{
    Q_OBJECT

public:
    Kirjanpito(QObject *parent = 0);


    ~Kirjanpito();

    /**
     * @brief Kirjanpidon asetuksen palauttaminen
     *
     * @deprecated Käytä asetukset()->asetus()
     *
     * @param avain
     * @return Asetuksen arvo
     */
    QString asetus(const QString& avain) const;

    /**
     * @brief Polku kirjanpitotiedostoon
     * @return
     */
    QString tiedostopolku() const { return polkuTiedostoon_;}

    /**
     * @brief Käytetäänkö harjoittelutilassa
     * @return tosi, jos harjoitellaan
     */
    bool onkoHarjoitus() const { return asetukset()->onko("Harjoitus"); }

    /**
     * @brief Nykyinen tai harjoittelutilassa muokattu päivämäärä
     *
     * Harjoittelutila mahdollistaa päivämäärän muuttamisen, jotta päästään testaamaan
     * tilinpäätökseen tms. liittyviä toimia
     * @return Päivämäärä
     */
    QDate paivamaara() const;

    /**
     * @brief Päivämäärä, johon saakka tilit on päätetty eli ei voi enää muokata
     * @return
     */
    QDate tilitpaatetty() const { return asetukset()->pvm("TilitPaatetty"); }

    Tilikausi tilikausiPaivalle(const QDate &paiva) const;

    /**
     * @brief Tositelajien model
     * @return
     */
    TositelajiModel *tositelajit() { return tositelajiModel_; }

    /**
     * @brief Asetusten model
     * @return
     */
    AsetusModel *asetukset() const { return asetusModel_; }

    /**
     * @brief Tilien model
     * @return
     */
    TiliModel *tilit() const { return tiliModel_; }

    /**
     * @brief Tilikausien model
     * @return
     */
    TilikausiModel *tilikaudet() const { return tilikaudetModel_; }

    /**
     * @brief Palauttaa TositeModel:in, jonka kautta pääsee tositteisiin
     * @return
     */
    TositeModel *tositemodel(QObject *parent = 0);

    /**
     * @brief Kohdennusten eli kustannuspaikkojen ja projektien model
     * @return
     */
    KohdennusModel *kohdennukset() const { return kohdennukset_; }

    /**
     * @brief Palauttaa alv-kirjauksen tyypit sisältävän modelin
     * @return
     */
    VerotyyppiModel *alvTyypit() const { return veroTyypit_; }

    /**
     * @brief Palauttaa tilityypit sisältävän modelin
     * @return
     */
    TilityyppiModel *tiliTyypit() const { return tiliTyypit_;}

    /**
     * @brief Palauttaa tuoteluettelon sisältävän modelin
     *
     * Tuoteluettelosta voidaan laskutuksessa valita valmiita tuotteita
     * @return
     */
    TuoteModel *tuotteet() const { return tuotteet_; }

    /**
     * @brief Sql-tietokanta
     *
     * Tätä käytetään, kun modelin arvot luetaan suoraan tietokannasta, siis
     * muokattaessa kun ei haluta muokata suoraan käytössä olevaa modelia
     *
     * @return
     */
    QSqlDatabase *tietokanta()  { return &tietokanta_; }

    /**
     * @brief QPrinter kaikenlaiseen tulosteluun
     * @return
     */
    QPrinter *printer() { return printer_;}

    /**
     * @brief Näyttää halutun ohjesivun selaimessa
     * @param ohjesivu
     */
    void ohje(const QString& ohjesivu = QString());

    /**
     * @brief Avaa url:n QDesktopServicellä
     *
     * Mahdollisten virhetilanteiden takia tehty käärefunktio, joka pyrkii QDesktopServices::openUrl()-funktiolla avaamaan
     * annetun urlin ja ellei se onnistu, näyttää virheilmoituksen
     *
     * @param url QUrl sijaintiin
     */
    static void avaaUrl(const QUrl &url);

    /**
     * @brief Tilapäistiedoston polku
     * @param Tilapäistiedoston nimi, XXXX korvataan satunnaisella jonolla
     * @return
     */
    QString tilapainen(QString nimi) const;

    /**
     * @brief Onko kyseisenä päivänä käytössä maksuperusteinen arvonlisävero
     * @param paiva
     * @return
     */
    bool onkoMaksuperusteinenAlv(const QDate& paiva) const;

    /**
     * @brief Palauttaa logon
     * @return
     */
    QImage logo() const { return logo_;}

    /**
     * @brief Asettaa kirjanpidon logon
     * @param logo
     */
    void asetaLogo(const QImage& logo);

    /**
     * @brief Liitteet ilman tositetta (esim. logo, tilinpäätökset)
     * @return
     */
    LiiteModel *liitteet() { return liitteet_;}

    /**
     * @brief Arkistohakemiston polku
     * @return
     */
    QString arkistopolku() const;

signals:
    /**
     * @brief Tietokanta on avattu
     *
     * Kirjanpito vaihtui ja kaikki tiedot ladataan uudelleen
     */
    void tietokantaVaihtui();
    /**
     * @brief Kirjanpitoa on muokattu (tallennettu muokattu vienti)
     */
    void kirjanpitoaMuokattu();

    /**
     * @brief Perusasetuksia muutetaan, joten aloitussivu päivitetään
     */
    void perusAsetusMuuttui();

    /**
     * @brief Kirjattavien kansio muuttui
     */
    void inboxMuuttui();

    /**
     * @brief Tilikausi on päätetty
     *
     * Päätetylle tilikaudelle ei voi enää kirjata mitään. Siksi tilikauden päättäminen vaikuttaa
     * eri valintoihin, ja siitä ilmoitetaan signaalilla.
     */
    void tilikausiPaatetty();

    /**
     * @brief Näytetään vähän aikaa ilmoitus onnistumisesta
     * @param teksti Näytettävä teksti
     */
    void onni(const QString& teksti);

    /**
     * @brief Pyytää näyttämään tositteen
     * @param tositeId
     */
    void naytaTosite(int tositeId);

public slots:
    /**
     * @brief Avaa kirjanpitotietokannan
     * @param tiedosto Kirjanpidon kitupiikki.sqlite-tiedoston täydellinen polku
     * @return tosi, jos onnistuu
     */
    bool avaaTietokanta(const QString& tiedosto);

    /**
     * @brief Lataa tietokannan uudelleen rakenteen muutoksen jälkeen
     * @return tosi, jos onnistui
     */
    bool lataaUudelleen();

    /**
     * @brief Asettaa päivämäärän, jota harjoittelutilassa eletään
     * @param pvm
     */
    void asetaHarjoitteluPvm(const QDate& pvm);


protected:
    QString polkuTiedostoon_;
    QSqlDatabase tietokanta_;
    QMap<QString,QString> viimetiedostot;
    QDate harjoitusPvm;

    TositelajiModel *tositelajiModel_;
    AsetusModel *asetusModel_;
    TiliModel *tiliModel_;
    TilikausiModel *tilikaudetModel_;
    KohdennusModel *kohdennukset_;
    VerotyyppiModel *veroTyypit_;
    TilityyppiModel *tiliTyypit_;
    TuoteModel *tuotteet_;
    LiiteModel *liitteet_;
    QPrinter *printer_;

    QTemporaryDir *tempDir_;
    QImage logo_;

public:
    /**
     * @brief Staattinen funktio, jonka kautta Kirjanpitoon päästään käsiksi
     *
     * Lyhyyden vuoksi voi käyttää myös globaalia kp()-funktiota
     *
     * @return
     */
    static Kirjanpito *db();

    /**
     * @brief Asetetaan pääohjelmassa ainokaisen instanssi
     *
     * Tähän siirryttiin, jotta tulee varmasti tuhotuksi
     *
     * @param kp
     * @since 0.5
     */
    static void asetaInstanssi(Kirjanpito* kp);

    /**
     * @brief Käytössä oleva tietokantaversio
     *
     * Jos yritetään avata uudempaa, tulee virhe
     */
    static const int TIETOKANTAVERSIO = 10;

    /**
     * @brief Palauttaa satunnaismerkkijonon
     * @param pituus
     * @return
     */
    static QString satujono(int pituus = 10);

private:
    static Kirjanpito *instanssi__;

    /**
     * @brief Suorittaa päivitykset
     * @param versioon Tietokantaversion (ei ohjelmaversio!)
     */
    void paivita(int versioon);
};

/**
 * @brief Globaali funktio kirjanpitoon pääsemiseksi
 *
 * Lyhenne funktiolle Kirjanpito::db()
 *
 * @return
 */
Kirjanpito* kp();


#endif // KIRJANPITO_H
