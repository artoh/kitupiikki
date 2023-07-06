#ifndef RAPORTTIVALINNAT_H
#define RAPORTTIVALINNAT_H

#include <QString>
#include <QVariantMap>
#include <QDate>

class RaporttiValintaSarake
{
public:
    enum SarakeTyyppi {
        Toteutunut,
        Budjetti,
        BudjettiEro,
        ToteumaProsentti
    };

    RaporttiValintaSarake();
    RaporttiValintaSarake(const QDate& loppuPvm);
    RaporttiValintaSarake(const QDate& alkuPvm, const QDate& loppuPvm, SarakeTyyppi tyyppi = Toteutunut);

    QDate alkuPvm() const { return alkuPvm_;}
    QDate loppuPvm() const { return loppuPvm_;}
    QDate pvm() const { return loppuPvm_;}
    SarakeTyyppi tyyppi() const { return tyyppi_;}


protected:
    QDate alkuPvm_;
    QDate loppuPvm_;
    SarakeTyyppi tyyppi_;
};


class RaporttiValinnat
{
public:

    enum Valinta {
        Tyyppi,
        Kieli,
        AlkuPvm,
        LoppuPvm,
        VientiJarjestys,
        TulostaKumppani,
        TulostaKohdennus,
        RyhmitteleTositelajit,
        TulostaSummarivit,
        ErittelePaivat,
        Kohdennuksella,
        Tililta,
        TulostaErittely,
        LuettelonTilit,
        LuetteloPvm,
        NaytaOtsikot,
        NaytaTyypit,
        SaldoPvm,
        NaytaKirjausohjeet,
        LaskuTyyppi,
        LaskuRajausTyyppi,
        VainAvoimet,
        LaskunLajittelu,
        NaytaViitteet,
        VainKitsaalla,
        RaportinMuoto,
        AlvAlkuPvm,
        AlvLoppuPvm,
        TiedostonNimi,
        TilausJarjestysNumero,
        TilinpaatosOtsikko,
        Kuukausittain,
        KuukaudetYhteensa
    };

    RaporttiValinnat();
    RaporttiValinnat(const QString& tyyppi);
    RaporttiValinnat(const RaporttiValinnat& toinen);

    void aseta(Valinta valinta, QVariant arvo = true);

    QVariant arvo(Valinta valinta) const { return valinnat_.value(valinta);};
    bool onko(Valinta valinta) const { return valinnat_.value(valinta, QVariant(false)).toBool();}

    void tyhjennaSarakkeet();
    void lisaaSarake(const RaporttiValintaSarake& sarake);
    void asetaSarakkeet(QList<RaporttiValintaSarake> sarakkeet);
    QList<RaporttiValintaSarake> sarakkeet() const { return sarakkeet_;}

    void nollaa();    
    QString nimi() const;

protected:
    QMap<Valinta,QVariant> valinnat_;
    QList<RaporttiValintaSarake> sarakkeet_;

};

#endif // RAPORTTIVALINNAT_H
