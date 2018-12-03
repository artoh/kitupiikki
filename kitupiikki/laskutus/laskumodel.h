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

#ifndef LASKUMODEL_H
#define LASKUMODEL_H


#include "db/tili.h"
#include "db/kohdennus.h"
#include "db/tositelaji.h"

#include "laskutmodel.h"

#include <QAbstractTableModel>
#include <QDate>
#include <QList>
#include <memory>

class LaskuRyhmaModel;

/**
 * @brief Laskun alv-erittelyn yksi rivi
 */
class AlvErittelyRivi
{
public:

    AlvErittelyRivi(int koodi = 0, int prosentti = 0);
    void lisaa(qlonglong netto,  qlonglong brutto);

    int alvKoodi() const { return alvKoodi_; }
    int alvProsentti() const { return alvProsentti_;}
    double netto() const { return  netto_;}
    double vero() const { return brutto_ - netto_; }
    double brutto() const { return brutto_;}

private:
    int alvKoodi_;
    int alvProsentti_;
    qlonglong netto_;
    qlonglong brutto_;

};


/**
 * @brief Laskun yksi rivi
 * 
 * Käytetään myös tuoteluettelon tuotteesta
 */
struct LaskuRivi
{
    LaskuRivi();
    qlonglong yhteensaSnt() const;
    qlonglong nettoSnt() const;

    QString nimike;
    double maara = 1.00;
    QString yksikko;
    double ahintaSnt = 0.00;
    int alvKoodi;
    int alvProsentti = 0;
    int aleProsentti = 0;
    Tili myyntiTili;
    Kohdennus kohdennus;
    int tuoteKoodi = 0;
    int voittoMarginaaliMenettely = 0;
};

/**
 * @brief Laskun tiedot
 */
class LaskuModel : public QAbstractTableModel
{
    Q_OBJECT

public:
    LaskuModel(QObject *parent = nullptr);

    static LaskuModel *teeHyvityslasku(int hyvitettavaVientiId);
    static LaskuModel *teeMaksumuistutus(int muistutettavaVientiId);
    static LaskuModel *haeLasku(int vientiId);
    /**
     * @brief Tekee laskusta kopion (uudella laskunumerolla ja päivämäärillä)
     * @param vientiId
     * @return
     */
    static LaskuModel *kopioiLasku(int vientiId);
    static LaskuModel *ryhmaLasku();



    enum LaskuSarake
    {
        NIMIKE, MAARA, YKSIKKO, AHINTA, ALE, ALV, TILI, KOHDENNUS, BRUTTOSUMMA
    };

    enum Kirjausperuste {SUORITEPERUSTE, LASKUTUSPERUSTE, MAKSUPERUSTE, KATEISLASKU};
    enum Laskutyppi { TUOTULASKU, LASKU, HYVITYSLASKU, MAKSUMUISTUTUS, OSTOLASKU, RYHMALASKU};


    enum
    {
        NimikeRooli = Qt::UserRole,
        AlvKoodiRooli = Qt::UserRole + 5,
        AlvProsenttiRooli = Qt::UserRole + 6,
        NettoRooli = Qt::UserRole + 7,
        VeroRooli = Qt::UserRole + 8,
        TuoteKoodiRooli = Qt::UserRole + 9,
        AHintaRooli = Qt::UserRole + 10,
        BruttoRooli = Qt::UserRole + 11,
        VoittomarginaaliRooli = Qt::UserRole + 12,
        AleProsenttiRooli = Qt::UserRole + 13,
        AlennusRooli = Qt::UserRole + 14
    };

    enum Voittomarginaalisyy {
        Kaytetyt = 310024,
        Taide = 320024,
        KerailyAntiikki = 330024
    };

    int rowCount(const QModelIndex &parent) const;
    int columnCount(const QModelIndex &parent) const;
    QVariant headerData(int section, Qt::Orientation orientation, int role) const;
    QVariant data(const QModelIndex &index, int role) const;
    bool setData(const QModelIndex &index, const QVariant &value, int role);
    Qt::ItemFlags flags(const QModelIndex &index) const;

    qlonglong laskunSumma() const;
    qlonglong nettoSumma() const;

    /**
     * @brief Palauttaa kopion laskurivistä
     * @param indeksi Rivin indeksi (alkane nollasta)
     * @return 
     */
    LaskuRivi rivi(int indeksi) const;
    
    QDate pvm() const;
    QDate erapaiva() const { return erapaiva_; }
    QDate toimituspaiva() const { return toimituspaiva_; }
    QString lisatieto() const { return lisatieto_;}
    QString osoite() const { return osoite_; }
    QString laskunsaajanNimi() const { return laskunsaajanNimi_; }
    int kirjausperuste() const { return kirjausperuste_;}
    QString email() const { return email_;}
    QString ytunnus() const { return ytunnus_; }
    QString verkkolaskuOsoite() const { return verkkolaskuOsoite_;}
    QString verkkolaskuValittaja() const { return verkkolaskuValittaja_;}
    QString asiakkaanViite() const { return asiakkaanViite_;}
    QString kieli() const { return kieli_; }
    /**
     * @brief Hyvityslaskulla hyvitettävä lasku ja maksumuistutuksella muistutettava
     * @return
     */
    AvoinLasku viittausLasku() const { return viittausLasku_; }


    qulonglong laskunro() const;
    QString viitenumero() const;
    Laskutyppi tyyppi() const { return tyyppi_; }
    qlonglong avoinSaldo() const { return avoinSaldo_; }
    bool onkoAlennuksia() const;
    int kplDesimaalit() const;

    /**
     * @brief Laskun arvonlisäveroerittely
     * @return
     */
    QList<AlvErittelyRivi> alverittely() const;

    LaskuRyhmaModel* ryhmaModel() { return ryhma_;}

    /**
     * @brief Hakee ryhmämodelista asiakkaan tiedot indeksiin
     * @param indeksi
     *
     * Näin ollen tulostettaessa, talletettaessa tai muussa vastaavassa
     * voidaan käsitellä tämä asiakas
     */
    void haeRyhmasta(int indeksi);


public slots:

    void asetaErapaiva(const QDate & paiva) { if(erapaiva_ != paiva) muokattu_ = true; erapaiva_ = paiva;  }
    void asetaLisatieto(const QString& tieto) { if(lisatieto_ != tieto) muokattu_ = true; lisatieto_ = tieto;  }
    void asetaOsoite(const QString& osoite) { if(osoite_ != osoite) muokattu_ = true; osoite_ = osoite;  }
    void asetaToimituspaiva(const QDate& pvm) { if(toimituspaiva_ != pvm) muokattu_ = true; toimituspaiva_ = pvm;  }
    void asetaLaskunsaajannimi(const QString& nimi) { if(laskunsaajanNimi_ != nimi) muokattu_ = true; laskunsaajanNimi_ = nimi;  }
    void asetaKirjausperuste(int kirjausperuste) { if(kirjausperuste_ != kirjausperuste) muokattu_ = true; kirjausperuste_ = kirjausperuste; }
    void asetaEmail(const QString& osoite) { if(email_ != osoite) muokattu_ = true; email_ = osoite; }
    void asetaYTunnus(const QString& ytunnus) { if(ytunnus != ytunnus) muokattu_ = true;  ytunnus_ = ytunnus; }
    void asetaAsiakkaanViite(const QString& viite) { if(asiakkaanViite() != viite) muokattu_ = true; asiakkaanViite_ = viite;}
    void asetaVerkkolaskuOsoite(const QString& osoite) { if(verkkolaskuOsoite() != osoite) muokattu_ = true; verkkolaskuOsoite_ = osoite;}
    void asetaVerkkolaskuValittaja(const QString& valittaja) { if(verkkolaskuValittaja() != valittaja) muokattu_=true; verkkolaskuValittaja_ = valittaja;}

public:

    /**
     * @brief Tallentaa tämän laskun, jonka jälkeen model pitäisi unohtaa
     * @return
     */
    bool tallenna(Tili rahatili, const QString& kieli = QString());

    /**
     * @brief Laskee viitenumeron tarkasteluvun
     * @param luvusta Viitenumero ilman tarkastetta
     * @return
     */
    static unsigned int laskeViiteTarkiste(qulonglong luvusta);

    /**
     * @brief Kirjanpidon tositetunnus
     *
     * Jos muokataan tositetta, on tositetunnus jo olemassa, muuten haetaan seuraava
     * @return
     */
    QString tositetunnus();


    bool muokattu() const  { return muokattu_; }

public slots:
    void lisaaRivi(LaskuRivi rivi = LaskuRivi());
    void poistaRivi(int indeksi);

signals:
    void summaMuuttunut(qlonglong summaSnt);

protected:
    void haeAvoinSaldo();

private:
    QList<LaskuRivi> rivit_;
    QDate erapaiva_;
    QDate toimituspaiva_;
    QString laskunsaajanNimi_;
    QString lisatieto_;
    QString osoite_;
    int kirjausperuste_;
    QString email_;
    QString ytunnus_;
    AvoinLasku viittausLasku_;
    Laskutyppi tyyppi_ = LASKU;
    int tositeId_ = 0;
    qulonglong laskunNumero_ = 0;
    int vientiId_ = 0;
    qlonglong avoinSaldo_ = 0;
    bool muokattu_ = false;
    QString asiakkaanViite_;
    QString verkkolaskuOsoite_;
    QString verkkolaskuValittaja_;
    QString kieli_;

    LaskuRyhmaModel* ryhma_ = nullptr;


    void paivitaSumma(int rivi);
};

#endif // LASKUMODEL_H
