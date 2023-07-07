#ifndef PROCOUNTORTUONTITIEDOSTO_H
#define PROCOUNTORTUONTITIEDOSTO_H


#include <QList>
#include <QDate>
#include "model/euro.h"
#include "tuonti/tilimuuntomodel.h"
#include "../tilinavausmodel.h"

class ProcountorTuontiTiedosto
{
protected:
    class SaldoTieto {
    public:
        SaldoTieto();
        SaldoTieto(const QString& tilinumero, const QString& tilinimi, const QList<Euro>& saldot);

        int tilinumero() const { return tilinumero_.toInt();}
        QString tili() const { return tilinumero_;}
        QString tilinimi() const { return tilinimi_;}
        QList<Euro> saldot() const { return saldot_;}
        Euro summa() const;
        void oikaiseAvauksesta(const Euro& euro);        

    protected:
        QString tilinumero_;
        QString tilinimi_;
        QList<Euro> saldot_;
    };


public:
    enum TiedostonTyyppi { TUNTEMATON = 0, TULOSLASKELMA = 0b00100, TASE = 0b01000};
    enum TiedostonKausi { VIRHEELLINEN = 0, EDELLINEN = 0b01, TAMA = 0b10 };
    enum TuontiStatus { TUONTI_OK, TIEDOSTO_VIRHE, PAIVAT_PUUTTUU, KAUDET_PUUTTUU, VIRHEELLISET_KAUDET, TIEDOSTO_TUNTEMATON };

    ProcountorTuontiTiedosto();
    TuontiStatus tuo(const QString& polku);

    void lisaaMuuntoon(TiliMuuntoModel* muunto);

    TiedostonTyyppi tyyppi() const { return tyyppi_;}
    TiedostonKausi kausi() const { return kausi_;}
    int kausia() const { return paivat_.count();}

    bool validi();
    bool oikaistavaTase() const { return oikaistavaTase_; }

    void tallennaTilinavaukseen(TilinavausModel* tilinavaus, TiliMuuntoModel* muunto);
    void oikaiseTilinavaus(const ProcountorTuontiTiedosto& edellinenTase);
    void oikaiseTili(const SaldoTieto& saldotieto);
    void oikaiseEdellinenTulos(const Euro& euroa, TiliMuuntoModel *muunto);
    void tallennaAlkutositteeseen(Tosite* tosite, TiliMuuntoModel* muunto);

    Euro tiedostoSumma() const;

protected:
    QDate paivaksi(const QString& teksti);

private:
    QList<QDate> paivat_;
    QList<SaldoTieto> saldot_;

    TiedostonTyyppi tyyppi_ = TUNTEMATON;
    TiedostonKausi kausi_ = VIRHEELLINEN;
    bool oikaistavaTase_ = false;


    static QRegularExpression tiliRE__;
    static QRegularExpression pvmRE__;
    static QRegularExpression kausiRE__;
};

#endif // PROCOUNTORTUONTITIEDOSTO_H
