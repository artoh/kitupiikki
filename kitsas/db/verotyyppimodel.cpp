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

#include "verotyyppimodel.h"

#include "db/kirjanpito.h"

VeroTyyppi::VeroTyyppi(AlvKoodi::Koodi uKoodi, const QString &uSelite, const QString &uKuvake, bool uNollalaji)
    : koodi(uKoodi), selite(uSelite), kuvake( QIcon(uKuvake)), nollalaji(uNollalaji)
{

}


VerotyyppiModel::VerotyyppiModel(QObject *parent)
    : QAbstractListModel( parent)
{
    tyypit.append( VeroTyyppi(AlvKoodi::EIALV,"Veroton",":/pic/tyhja.png",true));
    tyypit.append( VeroTyyppi(AlvKoodi::MYYNNIT_NETTO,"Verollinen myynti (netto)",":/pic/netto.png"));
    tyypit.append( VeroTyyppi(AlvKoodi::OSTOT_NETTO,"Verollinen osto (netto)", ":/pic/netto.png"));
    tyypit.append( VeroTyyppi(AlvKoodi::MAKSUPERUSTEINEN_MYYNTI, "Verollinen myynti (maksuperusteinen alv)",":/pic/euro.png"));
    tyypit.append( VeroTyyppi(AlvKoodi::MAKSUPERUSTEINEN_OSTO, "Verollinen osto (maksuperusteinen alv)",":/pic/euro.png"));    
    tyypit.append( VeroTyyppi(AlvKoodi::ENNAKKOLASKU_MYYNTI, "Ennakkolasku (myynti)",":/pic/lasku.png"));
    tyypit.append( VeroTyyppi(AlvKoodi::MYYNNIT_BRUTTO,"Verollinen myynti (brutto)",":/pic/lihavoi.png"));
    tyypit.append( VeroTyyppi(AlvKoodi::OSTOT_BRUTTO,"Verollinen osto (brutto)",":/pic/lihavoi.png"));
    tyypit.append( VeroTyyppi(AlvKoodi::ALV0,"Nollaverokannan alainen myynti",":/pic/0pros.png",true));
    tyypit.append( VeroTyyppi(AlvKoodi::MYYNNIT_MARGINAALI,"Voittomarginaalijärjestelmä, myynti",":/pic/marginaali.png"));
    tyypit.append( VeroTyyppi(AlvKoodi::OSTOT_MARGINAALI,"Voittomarginaalijärjestelmä, osto",":/pic/marginaali.png"));
    tyypit.append( VeroTyyppi(AlvKoodi::YHTEISOMYYNTI_TAVARAT,"Tavaroiden yhteisömyynti",":/pic/eu.png", true));
    tyypit.append( VeroTyyppi(AlvKoodi::YHTEISOMYYNTI_PALVELUT,"Palveluiden yhteisömyynti",":/pic/eu.png", true));
    tyypit.append( VeroTyyppi(AlvKoodi::YHTEISOHANKINNAT_TAVARAT,"Tavaroiden yhteisöhankinnat",":/pic/eu.png"));
    tyypit.append( VeroTyyppi(AlvKoodi::YHTEISOHANKINNAT_PALVELUT,"Palveluiden yhteisöhankinnat",":/pic/eu.png"));
    tyypit.append( VeroTyyppi(AlvKoodi::MAAHANTUONTI,"Tavaroiden maahantuonti EU:n ulkopuolelta",":/pic/laiva.png"));
    tyypit.append( VeroTyyppi(AlvKoodi::MAAHANTUONTI_VERO,"Tavaroiden maahantuonti, veron kirjaus",":/pic/laiva.png"));
    tyypit.append( VeroTyyppi(AlvKoodi::MAAHANTUONTI_PALVELUT, "Palveluostot EU:n ulkopuolelta", ":/pixaby/globe.png"));
    tyypit.append( VeroTyyppi(AlvKoodi::RAKENNUSPALVELU_MYYNTI,"Rakennuspalveluiden myynti",":/pic/vasara.png", true));
    tyypit.append( VeroTyyppi(AlvKoodi::RAKENNUSPALVELU_OSTO,"Rakennuspalveluiden osto",":/pic/vasara.png"));
    tyypit.append( VeroTyyppi(AlvKoodi::MAKSETTAVAALV,"Maksettavan arvonlisäveron tilitys",":/pic/vero.png", true));
    tyypit.append(VeroTyyppi(AlvKoodi::TILITYS,"Tekninen kirjaus",":/pic/vero.png", true));
    tyypit.append(VeroTyyppi(AlvKoodi::VAHENNYSKELVOTON,"Vähennyskelvottoman oston veron osuus", ":/pic/vero.png", true));

}

int VerotyyppiModel::rowCount(const QModelIndex & /*parent */) const
{
    return tyypit.count();
}

QVariant VerotyyppiModel::data(const QModelIndex &index, int role) const
{
    if( !index.isValid())
        return QVariant();

    VeroTyyppi tyyppi = tyypit.value(index.row());

    if( role == Qt::DisplayRole || role == SeliteRooli)
        return tyyppi.selite;
    else if( role == KoodiRooli)
        return tyyppi.koodi;
    else if( role == KoodiTekstiRooli )
        return QString::number(tyyppi.koodi);
    else if( role == Qt::DecorationRole)
        return tyyppi.kuvake;
    else if( role == NollaLajiRooli)
        return tyyppi.nollalaji;

    return QVariant();
}

QString VerotyyppiModel::seliteKoodilla(int koodi, const QString& kieli) const
{    

    foreach (VeroTyyppi tyyppi, tyypit)
    {
        if( tyyppi.koodi == koodi)
            return tulkkaa(tyyppi.selite, kieli);
    }
    return QString();
}

QString VerotyyppiModel::yhdistelmaSeliteKoodilla(int koodi, const QString& kieli) const
{
    if( koodi / 100 == 1)
        return tulkkaa("VERON MÄÄRÄ %1", kieli).arg( seliteKoodilla(koodi % 100, kieli) );
    else if( koodi / 100 == 2)
        return tulkkaa("VÄHENNYKSEN MÄÄRÄ %1", kieli).arg( seliteKoodilla(koodi % 100, kieli));
    else if( koodi / 100 == 4)
        return tulkkaa("KOHDENTAMATON MAKSUPERUSTEINEN %1", kieli).arg(seliteKoodilla(koodi % 100, kieli));
    else
        return seliteKoodilla( koodi , kieli);
}

QIcon VerotyyppiModel::kuvakeKoodilla(int koodi) const
{
    if( koodi < AlvKoodi::MAKSETTAVAALV)
        koodi = koodi % 100;

    foreach (VeroTyyppi tyyppi, tyypit)
    {
        if( tyyppi.koodi == koodi)
            return tyyppi.kuvake;
    }
    return QIcon();
}

bool VerotyyppiModel::nollaTyyppi(int koodi) const
{
    foreach (VeroTyyppi tyyppi, tyypit)
    {
        if( tyyppi.koodi == koodi)
            return tyyppi.nollalaji;
    }
    return false;
}

