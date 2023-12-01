#include "tiliotealvcombo.h"
#include "db/kirjanpito.h"

TilioteAlvCombo::TilioteAlvCombo(QWidget *parent) :
    QComboBox(parent)
{    
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

void TilioteAlvCombo::alustaTulolle()
{
    lisaa(AlvKoodi::EIALV);
    lisaa(AlvKoodi::MYYNNIT_NETTO, 24, "24 %");
    lisaa(AlvKoodi::MYYNNIT_NETTO, 14, "14 %");
    lisaa(AlvKoodi::MYYNNIT_NETTO, 10, "10 %");
    lisaa(AlvKoodi::ALV0);
    lisaa(AlvKoodi::MYYNNIT_MARGINAALI, 24);
    lisaa(AlvKoodi::YHTEISOMYYNTI_TAVARAT);
    lisaa(AlvKoodi::YHTEISOMYYNTI_PALVELUT);
    lisaa(AlvKoodi::RAKENNUSPALVELU_MYYNTI);
}

void TilioteAlvCombo::alustaMenolle()
{
    lisaa(AlvKoodi::EIALV);
    lisaa(AlvKoodi::OSTOT_NETTO, 24, "24 %");
    lisaa(AlvKoodi::OSTOT_NETTO, 14, "14 %");
    lisaa(AlvKoodi::OSTOT_NETTO, 10, "10 %");
    lisaa(AlvKoodi::OSTOT_MARGINAALI, 24);
    lisaa(AlvKoodi::YHTEISOHANKINNAT_TAVARAT, 24);
    lisaa(AlvKoodi::YHTEISOHANKINNAT_PALVELUT, 24);
    lisaa(AlvKoodi::RAKENNUSPALVELU_OSTO, 24);
    lisaa(AlvKoodi::MAAHANTUONTI_PALVELUT, 24);
}

void TilioteAlvCombo::lisaa(AlvKoodi::Koodi koodi, int prosentti, const QString &teksti)
{
    const int luku = koodi * 100 + prosentti;
    const QString selite = teksti.isEmpty() ? kp()->alvTyypit()->seliteKoodilla(koodi) : teksti ;
    addItem( kp()->alvTyypit()->kuvakeKoodilla(koodi), selite, luku);
}

