/*
   Copyright (C) 2018 Arto Hyvättinen

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

#include "tuonti.h"
#include "pdftuonti.h"
#include "kirjaus/kirjauswg.h"

Tuonti::Tuonti(KirjausWg *wg)
    : kirjausWg_(wg)
{

}

bool Tuonti::tuo(const QString &tiedostonnimi, KirjausWg *wg)
{

    if( tiedostonnimi.endsWith(".pdf", Qt::CaseInsensitive) )
    {
        PdfTuonti pdftuonti(wg);
        return pdftuonti.tuoTiedosto(tiedostonnimi);
    }

    return true;
}

void Tuonti::lisaaLasku( qlonglong sentit, QDate laskupvm, QDate erapvm, QString viite, QString tilinumero, QString saajanNimi)
{
    kirjausWg()->gui()->otsikkoEdit->setText(saajanNimi);
    kirjausWg()->gui()->tositePvmEdit->setDate(laskupvm);


    VientiRivi rivi;
    rivi.pvm = laskupvm;
    rivi.selite = saajanNimi;

    if( !tilinumero.isEmpty() &&  kp()->tilit()->tiliIbanilla(tilinumero).onkoValidi() )
    {
        // Oma tili eli onkin myyntilasku
        kirjausWg()->gui()->tositetyyppiCombo->setCurrentIndex(
                    kirjausWg()->gui()->tositetyyppiCombo->findData(TositelajiModel::MYYNTILASKUT, TositelajiModel::KirjausTyyppiRooli) );
        // Tähän pitäisi saada myyntisaatavien tili
        rivi.debetSnt = sentit;
    }
    else
    {
        kirjausWg()->gui()->tositetyyppiCombo->setCurrentIndex(
                    kirjausWg()->gui()->tositetyyppiCombo->findData(TositelajiModel::OSTOLASKUT, TositelajiModel::KirjausTyyppiRooli) );
        rivi.tili = kp()->tilit()->tiliTyypilla(TiliLaji::OSTOVELKA);
        rivi.kreditSnt = sentit;
    }

    rivi.viite = viite;
    rivi.saajanTili = tilinumero;
    rivi.erapvm = erapvm;
    rivi.json.set("SaajanNimi", saajanNimi);

    kirjausWg()->model()->vientiModel()->lisaaVienti(rivi);
    kirjausWg()->tiedotModeliin();

}
