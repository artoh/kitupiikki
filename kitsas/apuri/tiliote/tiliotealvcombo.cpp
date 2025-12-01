#include "tiliotealvcombo.h"
#include "db/kirjanpito.h"

TilioteAlvCombo::TilioteAlvCombo(QWidget *parent) :
    QComboBox(parent)
{
    lisaa(AlvKoodi::EIALV);
}

void TilioteAlvCombo::aseta(int koodi)
{
    int indeksi = findData( koodi );
    if( indeksi > -1)
        setCurrentIndex(indeksi);
}

int TilioteAlvCombo::koodi() const
{
    return currentData().toInt();
}

void TilioteAlvCombo::alustaTulolle(const QDate& pvm)
{    
    lisaa(AlvKoodi::MYYNNIT_NETTO, 2550, "25,5 %");
    lisaa(AlvKoodi::MYYNNIT_NETTO, 2400, "24 %");
    lisaa(AlvKoodi::MYYNNIT_NETTO, 1400, "14 %");
    lisaa(AlvKoodi::MYYNNIT_NETTO, 1350, "13,5 %");
    lisaa(AlvKoodi::MYYNNIT_NETTO, 1000, "10 %");
    lisaa(AlvKoodi::ALV0);
    lisaa(AlvKoodi::MYYNNIT_MARGINAALI, yleinenAlv(pvm));
    lisaa(AlvKoodi::YHTEISOMYYNTI_TAVARAT);
    lisaa(AlvKoodi::YHTEISOMYYNTI_PALVELUT);
    lisaa(AlvKoodi::RAKENNUSPALVELU_MYYNTI);
}

void TilioteAlvCombo::alustaMenolle(const QDate& pvm)
{
    lisaa(AlvKoodi::OSTOT_NETTO, 2550, "25,5%");
    lisaa(AlvKoodi::OSTOT_NETTO, 2400, "24 %");
    lisaa(AlvKoodi::OSTOT_NETTO, 1400, "14 %");
    lisaa(AlvKoodi::OSTOT_NETTO, 1350, "13,5 %");
    lisaa(AlvKoodi::OSTOT_NETTO, 1000, "10 %");
    lisaa(AlvKoodi::OSTOT_MARGINAALI, yleinenAlv(pvm));
    lisaa(AlvKoodi::YHTEISOHANKINNAT_TAVARAT, yleinenAlv(pvm));
    lisaa(AlvKoodi::YHTEISOHANKINNAT_PALVELUT, yleinenAlv(pvm));
    lisaa(AlvKoodi::RAKENNUSPALVELU_OSTO, yleinenAlv(pvm));
    lisaa(AlvKoodi::MAAHANTUONTI_PALVELUT, yleinenAlv(pvm));
}

void TilioteAlvCombo::lisaa(AlvKoodi::Koodi koodi, int sadasosaProsentti, const QString &teksti)
{
    const int luku = koodi + sadasosaProsentti * 100;
    const QString selite = teksti.isEmpty() ? kp()->alvTyypit()->seliteKoodilla(koodi) : teksti ;
    addItem( kp()->alvTyypit()->kuvakeKoodilla(koodi), selite, luku);
}

