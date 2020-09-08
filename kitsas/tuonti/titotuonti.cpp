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
#include <QDate>
#include <QByteArray>

namespace Tuonti {



TitoTuonti::TitoTuonti()
{

}

QVariantMap TitoTuonti::tuo(const QByteArray &data)
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
        return QVariantMap();

    QVariantMap map = ekarivi( rivit.takeFirst());
    QVariantList tapahtumat;

    // Sitten käydään tiliote lävitse    
    int tasotunnus = 0;

    QVariantMap rmap;
    QString selite;

    for( const QString& rivi : rivit )
    {        
        if( rivi.startsWith("T11"))
        {            
            // Täydentävää tietoa - tästä poimitaan saajan IBAN
            if( rivi.midRef(6,2) == "11")
            {
                map.insert("iban", rivi.mid(43,35).simplified());
                // Myös mahdollinen euromuotoinen viite
                if( rivi.midRef(8,35).startsWith("RF"))
                    rmap.insert("viite",rivi.mid(8,35).simplified());
            }
            // Vapaa lisätieto
            else if( rivi.midRef(6,2) == "00" )
            {
                // Koska OP ei käytä asianmukaisesti 11-tyypin tietuetta,
                // poimitaan viite myös 00-tietueesta
                if( rivi.midRef(8,35).startsWith("RF"))
                    rmap.insert("viite",rivi.mid(8,35).simplified());
                else
                {
                    if( !selite.isEmpty() && !selite.right(1).isEmpty())
                       selite.append(' ');
                    selite.append( rivi.mid(8));
                }

            }
            // Kappaletieto jos monta
            else if( rivi.midRef(6,2)== "01")
            {
                if( !selite.isEmpty() && !selite.right(1).isEmpty())
                    selite.append(' ');
                selite.append( QString("VIITEMAKSUJA %1 kpl").arg( rivi.midRef(8,8).toInt() ) );
            }

        }
        else
        {
            // Rivin tallentaminen
            rmap.insert("selite", selite);
            int rivintaso = rivi.mid(187,1).simplified().toInt();
            if( rmap.value("pvm").toDate().isValid() && rivintaso <= tasotunnus)
                tapahtumat.append(rmap);

            rmap.clear();
            selite.clear();
        }

        if( rivi.startsWith("T10") )
        {
            rmap.insert("pvm",QDate::fromString( rivi.mid(30,6), "yyMMdd" ).addYears(100));
            rmap.insert("ktokoodi", rivi.mid(49,3).toInt());
            qlonglong sentit = rivi.mid(88,18).toLongLong();
            if( rivi.at(87) == QChar('-'))
                sentit = 0-sentit;            
            rmap.insert("euro", sentit / 100.0);
            rmap.insert("saajamaksaja", rivi.mid(108,35).simplified());
            rmap.insert("viite",rivi.mid(159,20).simplified());
            rmap.insert("arkistotunnus", rivi.mid(12,18).simplified());
            tasotunnus = rivi.mid(187,1).simplified().toInt();
        }
    }
    map.insert("tapahtumat", tapahtumat);
    return map;
}

QVariantMap TitoTuonti::ekarivi(const QString &rivi)
{
    // Aloittaa tiliotteen tuomisen
    QVariantMap map;
    map.insert("tyyppi",400);
    map.insert("alkupvm", QDate::fromString( rivi.mid(26,6),"yyMMdd" ).addYears(100));
    map.insert("loppupvm", QDate::fromString( rivi.mid(32,6),"yyMMdd").addYears(100));
    map.insert("iban", rivi.mid(292,18));

    return map;

}

}
