/*
   Copyright (C) 2019 Arto Hyvättinen

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
#include "laskualvcombo.h"

#include "db/verotyyppimodel.h"
#include "db/kirjanpito.h"
#include "model/lasku.h"

LaskuAlvCombo::LaskuAlvCombo(QWidget *parent) :
    QComboBox (parent)
{

}

void LaskuAlvCombo::alusta(LaskuAlvCombo::AsiakasVeroLaji asiakasVerolaji, int alvkoodi, bool ennakkolasku, const QDate &pvm)
{
    addItem(QIcon(":/pic/netto-m.svg"),"10%", QVariant(AlvKoodi::MYYNNIT_NETTO + 10 * 10000 ));
    addItem(QIcon(":/pic/netto-m.svg"),"14%", QVariant(AlvKoodi::MYYNNIT_NETTO + 14 * 10000));
    addItem(QIcon(":/pic/netto-m.svg"),"24%", QVariant(AlvKoodi::MYYNNIT_NETTO + 24 * 10000 ));
    addItem(QIcon(":/pic/netto-m.svg"),"25,5%", QVariant(AlvKoodi::MYYNNIT_NETTO + 255000 ));
    addItem(QIcon(":/pic/tyhja.png"),tr("Veroton"), QVariant(AlvKoodi::EIALV ));
    addItem(QIcon(":/pic/0pros.png"),"Nollaverokannan alainen myynti", QVariant(AlvKoodi::ALV0));

    if( asiakasVerolaji == KAIKKI || asiakasVerolaji == EU || asiakasVerolaji == KOTIMAA || alvkoodi == AlvKoodi::RAKENNUSPALVELU_MYYNTI)
        addItem(QIcon(":/pic/vasara.png"), tr("Rakennuspalvelut"), QVariant( AlvKoodi::RAKENNUSPALVELU_MYYNTI ));

    if( asiakasVerolaji == EU || alvkoodi == AlvKoodi::YHTEISOMYYNTI_TAVARAT || alvkoodi == AlvKoodi::YHTEISOMYYNTI_PALVELUT) {
        addItem(QIcon(":/pic/eu.png"), tr("Tavaramyynti"), QVariant( AlvKoodi::YHTEISOMYYNTI_TAVARAT ));
        addItem(QIcon(":/pic/eu.png"), tr("Palvelumyynti"), QVariant( AlvKoodi::YHTEISOMYYNTI_PALVELUT ));
    }

    if( !kp()->onkoMaksuperusteinenAlv(pvm) && !ennakkolasku)
    {
        addItem(QIcon(":/pic/marginaali.png"),tr("Voittomarginaalimenettely - käytetyt tavarat"), QVariant(Lasku::KAYTETYT));
        addItem(QIcon(":/pic/marginaali.png"),tr("Voittomarginaalimenettely - taide-esineet"), QVariant(Lasku::TAIDE));
        addItem(QIcon(":/pic/marginaali.png"),tr("Voittomarginaalimenettely - keräily- ja antiikkiesineet"), QVariant(Lasku::ANTIIKKI));
    }

    if( kp()->asetukset()->onko("AlvVelvollinen") ) {
        setCurrentIndex(3); // Alv 24
    } else {
        setCurrentIndex(4);  // Veroton
    }
}

void LaskuAlvCombo::aseta(int alvkoodi, double alvprosentti)
{
    int koodi = alvkoodi + qRound(alvprosentti * 10000);
    int indeksi = findData( koodi );
    if(indeksi > -1)
        setCurrentIndex( indeksi );
}

int LaskuAlvCombo::veroKoodi() const
{   
    return currentData().toInt() % 100;
}

double LaskuAlvCombo::veroProsentti() const
{
    if( veroKoodi() == AlvKoodi::MYYNNIT_NETTO)
        return (currentData().toInt() / 100) / 100.0;
    else
        return 0.0;
}
