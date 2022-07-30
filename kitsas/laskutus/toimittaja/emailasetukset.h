#ifndef EMAILASETUKSET_H
#define EMAILASETUKSET_H

#include <QString>

class EmailAsetukset
{
public:
    enum {
        ASETUKSET_TIETOKONE = 0,
        ASETUKSET_KIRJANPITO = 1,
        KITSAS_PALVELIN = 2
    };

    EmailAsetukset();

    QString palvelin() const {return palvelin_;}
    QString kayttaja() const {return kayttaja_;}
    QString salasana() const {return salasana_;}
    QString lahettajaNimi() { return lahettajaNimi_;}
    QString lahettajaOsoite() { return lahettajaOsoite_;}
    QString kopioOsoite() { return kopioOsoite_;}
    int portti() const { return portti_;}
    int suojaus() const { return suojaus_;}

    int asetustapa() const { return asetustapa_;}
    bool kitsasPalvelin() { return asetustapa_ == KITSAS_PALVELIN;}
    bool kayttajakohtainen() const { return kayttajakohtainen_;}

    void setPalvelin(const QString& palvelin) { palvelin_ = palvelin;}
    void setKayttaja(const QString& kayttaja) { kayttaja_ = kayttaja;}
    void setSalasana(const QString& salasana) { salasana_ = salasana;}
    void setLahettajaNimi(const QString& nimi) { lahettajaNimi_ = nimi;}
    void setLahettajaOsoite(QString osoite);
    void setKopioOsoite(const QString& osoite) { kopioOsoite_ = osoite;}
    void setPortti(int portti) { portti_ = portti;}
    void setSuojaus(int suojaus) { suojaus_ = suojaus;}
    void setAsetusTapa(int asetustapa) { asetustapa_ = asetustapa;}
    void setKayttajaKohtainen(bool onko) { kayttajakohtainen_ = onko;}

    void lataa();
    void tallenna();




protected:
    int asetustapa_ = ASETUKSET_TIETOKONE;
    bool kayttajakohtainen_ = false;

    QString palvelin_;
    QString kayttaja_;
    QString salasana_;
    QString lahettajaNimi_;
    QString lahettajaOsoite_;
    QString kopioOsoite_;
    int portti_;
    int suojaus_;

    void lataaTietokoneAsetukset();
    void lataaKirjanpitoAsetukset();
    void lataaKitsasAsetukset();
    bool lataaOmaEmail();

    void tallennaPaikallinen();
    void tallennaKirjanpito();
    bool tallennaOmaEmail();

    static int sslIndeksi(const QString& asetus);
    static QString sslAsetus(int indeksi);
};

#endif // EMAILASETUKSET_H
