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
#include <QStringList>

#include "tilikausi.h"

#include "asetusmodel.h"
#include "tilikausimodel.h"
#include "kohdennusmodel.h"
#include "verotyyppimodel.h"
#include "tilityyppimodel.h"



#include "tilimodel.h"
#include "tili.h"

#include "laskutus/tuotemodel.h"

#include "kpkysely.h"

class QPrinter;
class QSettings;

class PilviModel;
class SQLiteModel;
class YhteysModel;

class TositeTyyppiModel;
class RyhmatModel;
class AlvIlmoitustenModel;
class KiertoModel;

class QNetworkAccessManager;
class Tulkki;

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
    Kirjanpito(const QString& portableDir = QString());
    ~Kirjanpito();

    enum Onni { Onnistui = 0, Verkkovirhe = 2, Stop=3, Haetaan = 4 };

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
    QString kirjanpitoPolku();

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
     * @brief Tositteiden tyyppien model
     * @return
     */
    TositeTyyppiModel *tositeTyypit() const { return  tositeTyypit_;}

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
    RyhmatModel *ryhmat() const { return ryhmat_;}
    AlvIlmoitustenModel *alvIlmoitukset() const { return alvIlmoitukset_;}
    KiertoModel *kierrot() const { return kiertoModel_; }

    /**
     * @brief Sql-tietokanta
     *
     * Tätä käytetään, kun modelin arvot luetaan suoraan tietokannasta, siis
     * muokattaessa kun ei haluta muokata suoraan käytössä olevaa modelia
     *
     * @return
     */
    [[deprecated]] QSqlDatabase *tietokanta()  { return &tietokanta_; }

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
     * @brief QSettings käyttäjäkohtaisille asetuksille
     * @return
     */
    QSettings* settings() { return settings_;}


    /**
     * @brief Yhteys taustajärjestelmään
     * @return
     */
    YhteysModel* yhteysModel() { return yhteysModel_; }


    /**
     * @brief Palauttaa tositetunnuksen muodossa OL 1/19
     * @param tositelaji
     * @param tunniste
     * @param pvm
     * @return
     */
    [[deprecated]]  QString tositeTunnus(int tunniste, const QDate& pvm, bool vertailu = false);

    QString tositeTunnus(int tunniste, const QDate& pvm, const QString& sarja, bool samakausi = false, bool vertailu = false);


    /**
     * @brief Kirjanpito on avattu
     * @param model
     *
     * Vaihtaa avatun kirjanpidon näkymään
     */
    void yhteysAvattu(YhteysModel *model);

    /**
     * @brief Kääntää tekstin tulkki.json-tiedoston avulla
     * @param teksti Käännettävä teksti
     * @param kieli Kielikoodi (fi, sv, en)
     * @return Käännetty teksti
     */
    QString kaanna(const QString& teksti, const QString& kieli = QString()) const;

    void asetaTositeSarjat(const QStringList& sarjat) { tositesarjat_=sarjat;}
    QStringList tositeSarjat() const { return tositesarjat_;}


    void odotusKursori(bool on);
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
     * @brief Tilikausi on avattu tai poistettu
     *
     * Päivittää luettelot, joissa tilikausia
     */
    void tilikausiAvattu();

    /**
     * @brief Näytetään vähän aikaa ilmoitus (epä)onnistumisesta
     * @param teksti Näytettävä teksti
     */
    void onni(const QString& teksti, Onni onni = Onnistui);

    /**
     * @brief Näytetään ilmoitus tositteen tallentumisesta
     * @param tunnus Tositteen tunnistenumero
     * @param paiva Tositteen päivämäärä
     * @param sarja Tositesarja
     */
    void tositeTallennettu(int tunnus, const QDate& paiva, const QString& sarja, int tila);
    void piilotaTallennusWidget();

    /**
     * @brief Pyytää näyttämään tositteen
     * @param tositeId
     */
    void naytaTosite(int tositeId);

    /**
     * @brief Tietokantavirhe on tapahtunut
     * @param virheilmoitus
     */
    void tietokantavirhe(QString virheilmoitus);  

    void logoMuuttui();


public slots:
    /**
     * @brief Avaa kirjanpitotietokannan
     * @param tiedosto Kirjanpidon kitupiikki.sqlite-tiedoston täydellinen polku
     * @return tosi, jos onnistuu
     */
    bool avaaTietokanta(const QString& tiedosto, bool ilmoitaVirheesta = true);

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

    void logoSaapui(QVariant *reply);


protected:
    QString polkuTiedostoon_;
    QSqlDatabase tietokanta_;
    QMap<QString,QString> viimetiedostot;
    QDate harjoitusPvm;

    AsetusModel *asetusModel_;
    TiliModel *tiliModel_;
    TilikausiModel *tilikaudetModel_;
    KohdennusModel *kohdennukset_;
    VerotyyppiModel *veroTyypit_;
    TilityyppiModel *tiliTyypit_;
    TuoteModel *tuotteet_;
    RyhmatModel *ryhmat_;
    AlvIlmoitustenModel *alvIlmoitukset_;
    QPrinter *printer_;

    QTemporaryDir *tempDir_;
    QImage logo_;

    QSettings* settings_;
    QString portableDir_;      // Portable-ohjelman käynnistyshakemisto

    QStringList virheloki_;

    QNetworkAccessManager* networkManager_;

    PilviModel *pilviModel_;
    SQLiteModel *sqliteModel_;

    YhteysModel *yhteysModel_;
    TositeTyyppiModel *tositeTyypit_;
    KiertoModel* kiertoModel_;

    QStringList tositesarjat_;

    Tulkki* tulkki_;

    bool waitCursor_ = false;


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
     * @brief Palauttaa satunnaismerkkijonon
     * @param pituus
     * @return
     */
    static QString satujono(int pituus = 10);

    /**
     * @brief Portable-ohjelman käynnistyshakemisto
     * @return Tyhjä, jos ei portable
     */
    QString portableDir() const { return portableDir_;}

    /**
     * @brief QNetworkAccessManager verkkoyhteyksiä varten
     * @return
     */
    QNetworkAccessManager* networkManager() { return networkManager_; }

    PilviModel* pilvi() { return pilviModel_; }
    SQLiteModel* sqlite() { return sqliteModel_; }

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

/**
 * @brief Globaali funktion kyselyn luomiseksi
 * @param polku
 * @param metodi
 * @return
 */
KpKysely *kpk(const QString& polku = QString(), KpKysely::Metodi metodi = KpKysely::GET);


/**
 * @brief Globaali funktio kielen/maan lippuun
 * @param kielikoodi
 * @return
 */
QIcon lippu(const QString& kielikoodi);

/**
 * @brief Globaali funktion tulkki.json-käännökseen
 * @param teksti Käännettävä teksti
 * @param kieli Kielikoodi (fi, sv, en)
 * @return Käännetty teksti
 */
QString tulkkaa(const QString& teksti, const QString& kieli = QString());

#endif // KIRJANPITO_H
