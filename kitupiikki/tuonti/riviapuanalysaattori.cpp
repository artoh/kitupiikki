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

#include "riviapuanalysaattori.h"

#include "validator/ibanvalidator.h"
#include "validator/viitevalidator.h"

#include <QRegularExpression>
#include <QRegularExpressionMatch>
#include <QDebug>

RiviApuAnalysaattori::RiviApuAnalysaattori()
    : maara_(0)
{

}

void RiviApuAnalysaattori::analysoi(QStringList rivi)
{
    // Ongelma, kun arkistointitunnus on viitemuodossa
    // Pitäisikö analysoida arkistointitunnuksen paikka???
    // Ehkä sitten sijainneilla merkitty lista???


    QRegularExpression rahaRe("(?<etu>[+-])?(?<eur>(\\d+[ .])*\\d+),(?<snt>\\d{2})(?<taka>[+-])?");
    QRegularExpression viiteRe("\\b(RF\\d{2}\\d{4,20}|\\d{4,20})\\b");
    QRegularExpression arkistoRe("\\b\\d{4}[0-9A-Z]{6,}\\b");
    QRegularExpression seliteRe("\\b[A-ö ]{8,}\\b");
    QRegularExpression pvmRe("(?<p>\\d{1,2})\\.(?<k>\\d{1,2})\\.(?<v>\\d{2}\\d{2}?)");


    IbanValidator ibanValidoija;
    ViiteValidator viiteValidoija;
    QRegularExpression ibanRe("\\b[A-Z]{2}\\d{2}\\w{6,30}\\b");

    int position = 0;

    kirjauspvm_ = QDate();
    iban_.clear();
    viite_.clear();
    arkistotunnus_.clear();
    maara_ = 0;
    selite_.clear();

    // Käydään läpi rivin kentät ja laitetaan ne paikoilleen

    for( QString teksti : rivi)
    {
        if( teksti.contains( pvmRe ) )
        {
            QRegularExpressionMatch mats = pvmRe.match(teksti);
            int vuosi = mats.captured("v").toInt();
            if( vuosi < 100)
                vuosi += QDate::currentDate().year() / 100;
            kirjauspvm_ = QDate( vuosi, mats.captured("k").toInt(), mats.captured("p").toInt());
        }
        if( teksti.contains( ibanRe ))
        {
            QRegularExpressionMatch mats = ibanRe.match(teksti);
            QString ehdokas = mats.captured(0);
            if( ibanValidoija.validate(ehdokas,position) == IbanValidator::Acceptable )
            {
                iban_ = ehdokas;
            }
        }
        if( teksti.contains( rahaRe))
        {
            QRegularExpressionMatch mats = rahaRe.match(teksti);
            // +/- ennen tai jälkeen
            // qDebug() << mats.captured() << " " << mats.captured("etu") << "|" << mats.captured("taka");

            if( mats.captured("etu") != mats.captured("taka"))
            {
                QString eurot = mats.captured("eur");
                eurot.replace(QRegularExpression("\\D"),"");
                maara_ = eurot.toInt() * 100 + mats.captured("snt").toInt();
                if( mats.captured("etu") == '-'  || mats.captured("taka") == '-')
                    maara_ = 0 - maara_;
            }

        }

        if( teksti.contains( viiteRe ) && viite().isEmpty())
        {
            QRegularExpressionMatch mats = viiteRe.match(teksti);
            QString ehdokas = mats.captured(0);
            if( viiteValidoija.validate(ehdokas,position) == ViiteValidator::Acceptable)
            {
                viite_ = ehdokas;
            }
        }
        if( teksti.contains(arkistoRe) && arkistotunnus().isEmpty())
        {
            QRegularExpressionMatch mats = arkistoRe.match(teksti);
            if( mats.captured() != viite() )
                arkistotunnus_ = mats.captured();
        }
        // Nyt meillä on jäljellä mahdollinen selite
        // Selitteessä pitää olla isoja ja pieniä
        if( selite().isEmpty() && teksti.contains(seliteRe))
        {
            QRegularExpressionMatch mats = seliteRe.match(teksti);
            QString ehdokas = mats.captured().simplified();

            // Selitteeksi ei oteta pankkitermejä
            if( !ehdokas.contains("SIIRTO") &&
                !ehdokas.contains("OSTO") &&
                !ehdokas.contains("LASKU") &&
                !ehdokas.contains("IBAN") &&
                !ehdokas.contains("BIC") &&
                !ehdokas.contains("ARKISTOINTITUNNUS", Qt::CaseInsensitive) &&
                !ehdokas.contains("TILINUMERO", Qt::CaseInsensitive) &&
                 ehdokas.length() > 8)
                selite_ = ehdokas;
        }

    }

}
