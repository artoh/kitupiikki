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
#include "iban.h"

Iban::Iban()
{

}

Iban::Iban(const QString &tilinumero)
{
    tilinumero_ = tilinumero;
    tilinumero_.remove(' ');
}

QString Iban::bic() const
{
    // Pitää olla suomalainen IBAN
    if( !tilinumero_.startsWith("FI"))
        return QString();

    // Elokuun 2020 tilanteen mukaan
    QString tunnus = tilinumero_.mid(4);

    if( tunnus.startsWith("405") || tunnus.startsWith("497"))
        return "HELSFIHH";  // Aktia Pankki
    else if(tunnus.startsWith("714"))
        return "EVSEFIHH";
    else if( tunnus.startsWith('8') )
        return "DABAFIHH";  // Danske Bank
    else if( tunnus.startsWith("34"))
        return "DABAFIHX";
    else if( tunnus.startsWith("31"))
        return "HANDFIHH";  // Handelsbanken
    else if( tunnus.startsWith('1') || tunnus.startsWith('2'))
        return "NDEAFIHH";  // Nordea
    else if( tunnus.startsWith('5'))
        return "OKOYFIHH";  // Osuuspankit
    else if( tunnus.startsWith("39") || tunnus.startsWith("36"))
        return "SBANFIHH";  // S-Pankki
    else if( tunnus.startsWith('6'))
        return "AABAFI22";  // Ålandsbanken
    else if( tunnus.startsWith("47") )
        return "POPFFI22"; // POP Pankit
    else if( tunnus.startsWith("715") || tunnus.startsWith('4'))
        return "ITELFIHH"; // Säästöpankkiryhmä
    else if( tunnus.startsWith("717"))
        return "BIGKFIH1";
    else if( tunnus.startsWith("713"))
        return "CITIFIHX";
    else if( tunnus.startsWith("37"))
        return "DNBAFIHX";
    else if( tunnus.startsWith("799"))
        return "HOLVFIHH";
    else if( tunnus.startsWith("33"))
        return "ESSEFIHX";
    else if( tunnus.startsWith("38"))
        return "SWEDFIHH";
    else if(tunnus.startsWith("798"))
        return "VPAYFIH2";


    // Tuntematon pankkikoodi
    return QString();
}

QString Iban::pankki() const
{
    QString b = bic();
    if( b == "HELSFIHH") return "Aktia";
    else if( b == "BIGKFIH1") return "Bigbank";
    else if( b == "EVSEFIHH") return "Alisa Pankki";
    else if( b == "CITIFIHX") return "Citibank";
    else if( b == "DABAFIHH" || b == "DABAFIHX" ) return "Danske Bank";
    else if( b == "DNBAFIHX") return "DnB NOR Bank";
    else if( b == "HANDFIHH") return "Handelsbanken";
    else if( b == "HOLVFIHH") return "Holvi";
    else if( b == "NDEAFIHH") return "Nordea";
    else if( b == "ITELFIHH") return "Säästöpankki";
    else if( b == "OKOYFIHH") return "Osuuspankki";
    else if( b == "POPFFI22") return "POP Pankki";
    else if( b == "SBANFIHH") return "S-Pankki";
    else if( b == "ESSEFIHX") return "SEB";
    else if( b == "AABAFI22") return "Ålandsbanken";
    else if( b == "SWEDFIHH") return "Swedbank";
    else if( b == "VPAYFIH2") return "Viva Payment";
    return QString();
}

QString Iban::valeilla() const
{
    return lisaaValit(tilinumero_);
}

QString Iban::valeitta() const
{
    return tilinumero_;
}

bool Iban::isValid() const
{
    if( tilinumero_.startsWith("FI") && tilinumero_.length() != 18)
        return false;
    if( tilinumero_.length() < 10 || tilinumero_.length() > 32)
        return false;
    if( !tilinumero_.at(2).isDigit() || !tilinumero_.at(3).isDigit())
        return false;
    if( !tilinumero_.at(0).isLetter() || !tilinumero_.at(1).isLetter())
        return false;

    return ibanModulo(tilinumero_) == 1;
}

QString Iban::lisaaValit(const QString &iban)
{
    QString raaka(iban);
    QString palautettava;

    for(int i=0; i < raaka.length(); i++)
    {
        palautettava.append(raaka.at(i));
        if( i % 4 == 3)
            palautettava.append(QChar(' '));
    }
    return palautettava;
}

int Iban::ibanModulo(const QString &iban)
{
    // Siirretään neljä ensimmäistä merkkiä loppuun
    QString siirto = iban.mid(4) + iban.left(4);

    // Muunnetaan kirjaimet numeropareiksi
    QString apu;
    for( const QChar& merkki : qAsConst( siirto ))
    {
        if( merkki.isDigit() )
            apu.append(merkki);
        else if( merkki.isUpper())
            apu.append( QString("%1").arg( static_cast<int>( merkki.toLatin1() ) - 55 , 2, 10, QChar('0')  ) );
        else
            return -1;
    }

    QString eka = apu.left(9);
    qlonglong luku = eka.toLongLong();
    int jaannos = luku % 97;

    if( apu.length() < 10 )
        return jaannos;

   apu.remove(0,9);

   while( apu.length() )
   {
       QString tama = QString("%1").arg( jaannos , 2, 10, QChar('0')  );
       tama.append( apu.left(7));
       luku = tama.toLongLong();
       jaannos = luku % 97;

       if( apu.length() > 6)
           apu.remove(0,7);
       else
           apu.clear();
   }

    return jaannos;

}

