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

#include "titotuonti.h"


TitoTuonti::TitoTuonti(KirjausWg *wg) :
    Tuonti( wg )
{

}

bool TitoTuonti::tuo(const QByteArray &data)
{
    // Haistellaan tiedoston koodaus
    QString str = QString::fromLocal8Bit(data);

    // Skandit ikivanhan ISO 646-koodauksen mukaan
    str.replace("]", "Å");
    str.replace("[","Ä");
    str.replace("\\", "Ö");
    str.replace("}","å");
    str.replace("{","ä");
    str.replace("|","ö");

    QStringList rivit = str.split("\r\n");

    if( !rivit.count())
        return false;

    ekarivi( rivit.takeFirst());

    // Sitten käydään tiliote lävitse
    QDate pvm;
    qlonglong sentit = 0;
    QString iban;
    QString viite;
    QString arkistotunnus;
    QString selite;

    for( const QString& rivi : rivit )
    {
        if( rivi.startsWith("T11"))
        {
            // Täydentävää tietoa - tästä poimitaan saajan IBAN
            if( rivi.midRef(6,2) == "11")
            {
                iban = rivi.mid(43,35).simplified();
                // Myös mahdollinen euromuotoinen viite
                if( rivi.midRef(8,35).startsWith("RF"))
                    viite = rivi.mid(8,35).simplified();
            }

        }
        else
        {
            // Rivin tallentaminen
            if( pvm.isValid())
                oterivi(pvm, sentit, iban, viite, arkistotunnus, selite);

            pvm = QDate();
            sentit = 0;
            iban.clear();
            viite.clear();
            arkistotunnus.clear();
            selite.clear();
        }

        if( rivi.startsWith("T10") )
        {
            pvm = QDate::fromString( rivi.mid(30,6), "yyMMdd" ).addYears(100);
            sentit = rivi.mid(88,19).toLongLong();
            if( rivi.at(87) == QChar('-'))
                sentit = 0-sentit;
            viite = rivi.mid(159,20);
            arkistotunnus = rivi.mid(12,18).simplified();
            selite = rivi.mid(108,35).simplified();
        }
    }
    return false;
}

void TitoTuonti::ekarivi(const QString &rivi)
{
    // Aloittaa tiliotteen tuomisen
    QDate mista = QDate::fromString( rivi.mid(26,6),"yyMMdd" ).addYears(100);
    QDate mihin = QDate::fromString( rivi.mid(32,6),"yyMMdd").addYears(100);
    QString iban = rivi.mid(292,18);
    tiliote(iban, mista, mihin);
}
