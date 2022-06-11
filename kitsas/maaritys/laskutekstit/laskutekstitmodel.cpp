#include "laskutekstitmodel.h"

#include "db/kirjanpito.h"

LaskuTekstitModel::LaskuTekstitModel(QObject *parent)
    : QAbstractListModel(parent)
{

    lisaa("Saate", tr("Saateviesti"), tr("Kun lasku lähetetään sähköpostilla, sähköpostiviestin otsikko ja sisältö"), true, true);
    lisaa("Lisatiedot", tr("Laskun lisätiedot"), tr("Jokaiselle laskulle oletuksena lisättävät lisätiedot laskun alussa"));
    lisaa("Maksumuistutussaate", tr("Maksumuistutusviesti"), tr("Kun maksumuistutus lähetetään sähköpostilla, sähköpostiviestin otsikko ja sisältö"), true, true);
    lisaa("Maksumuistutuslisatiedot", tr("Maksumuistutuksen lisätiedot"), tr("Maksumuistutuksille oletuksena lisättävät lisätiedot laskun alussa"));

}

int LaskuTekstitModel::rowCount(const QModelIndex &parent) const
{
    // For list models only the root node (an invalid parent) should return the list's size. For all
    // other (valid) parents, rowCount() should return 0 so that it does not become a tree model.
    if (parent.isValid())
        return 0;

    return tekstit_.count();
}

QVariant LaskuTekstitModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid())
        return QVariant();


    if( role == Qt::DisplayRole)
        return tekstit_.at(index.row()).nimi();

    return QVariant();
}

QString LaskuTekstitModel::kuvaus(int rivi) const
{
    return tekstit_.at(rivi).kuvaus();
}

QString LaskuTekstitModel::otsikko(int rivi, const QString &kieli) const
{
    return tekstit_.at(rivi).otsikko(kieli);
}

QString LaskuTekstitModel::sisalto(int rivi, const QString &kieli) const
{
    return tekstit_.at(rivi).sisalto(kieli);
}

bool LaskuTekstitModel::onkoOtsikkoa(int rivi) const
{
    return tekstit_.at(rivi).liput() & LaskuTeksti::OTSIKKO;
}

bool LaskuTekstitModel::onkoKorostusta(int rivi) const
{
    return tekstit_.at(rivi).liput() & LaskuTeksti::EMAILKOROSTUS;
}

void LaskuTekstitModel::asetaOtsikko(int rivi, const QString &kieli, const QString &otsikko)
{
    tekstit_[rivi].asetaOtsikko(kieli, otsikko);
}

void LaskuTekstitModel::asetaSisalto(int rivi, const QString &kieli, const QString &sisalto)
{
    tekstit_[rivi].asetaSisalto(kieli, sisalto);
}

bool LaskuTekstitModel::muokattu()
{
    for( auto& teksti : tekstit_) {
        if( teksti.muokattu()) return true;
    }
    return false;
}

void LaskuTekstitModel::nollaa()
{
    for( auto& teksti : tekstit_) {
        teksti.nollaa();
    }
}

void LaskuTekstitModel::tallenna()
{
    for( auto& teksti : tekstit_) {
        teksti.tallenna();
    }
}

void LaskuTekstitModel::lisaa(const QString &tunnus, const QString &nimi, const QString &kuvaus, bool otsikot, bool korostus)
{
    tekstit_.append( LaskuTeksti(tunnus, nimi, kuvaus, (otsikot * LaskuTeksti::OTSIKKO) + (korostus * LaskuTeksti::EMAILKOROSTUS) ) );
}

LaskuTekstitModel::LaskuTeksti::LaskuTeksti()
{

}

LaskuTekstitModel::LaskuTeksti::LaskuTeksti(const QString &tunnus, const QString& nimi, const QString &kuvaus, int liput) :
    tunnus_(tunnus), nimi_(nimi), kuvaus_(kuvaus), liput_(liput)
{

}

void LaskuTekstitModel::LaskuTeksti::nollaa()
{    
    QString sisalto = kp()->asetukset()->asetus("Laskuteksti/" + tunnus_);
    sisalto_ladattu_.aseta(sisalto);
    sisalto_muokattu_.aseta(sisalto);
    if( liput_ & LaskuTeksti::OTSIKKO) {
        QString lotsikko = kp()->asetukset()->asetus("Laskuteksti/" + tunnus_ + "_otsikko");
        otsikko_ladattu_.aseta(lotsikko);
        otsikko_muokattu_.aseta(lotsikko);
    }
}

void LaskuTekstitModel::LaskuTeksti::tallenna()
{
    if( sisalto_ladattu_.toString() != sisalto_muokattu_.toString()) {
        kp()->asetukset()->aseta("Laskuteksti/" + tunnus_, sisalto_muokattu_.toString());
        sisalto_ladattu_ = sisalto_muokattu_;
    }
    if( otsikko_ladattu_.toString() != otsikko_muokattu_.toString()) {
        kp()->asetukset()->aseta("Laskuteksti/" + tunnus_ + "_otsikko", otsikko_muokattu_.toString());
        otsikko_ladattu_ = otsikko_muokattu_;
    }
}
