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
#include "viitenumero.h"
#include "validator/ibanvalidator.h"

ViiteNumero::ViiteNumero()
{

}

ViiteNumero::ViiteNumero(const QString &viite) :
    viitenumero_(viite)
{
    if( viite.length() > 2 ) {

        QString siivottu = viite;
        siivottu.remove(" ");

        const QString pohja = siivottu.startsWith("RF")
                  ? siivottu.mid(4, siivottu.length()-5)
                  : siivottu.left(siivottu.length() - 1);

        if( pohja.length() < 6 ) {
            // Lyhyt vakioviite
            tyyppi_ = VAKIOVIITE;
            kanta_ = pohja;
        } else if( pohja.endsWith("000")) {
            // Vanha viite
            tyyppi_ = LASKU;
            kanta_ = pohja.left( pohja.length() - 3 );
        } else if(pohja.endsWith('9')) {
            // Lyhyt viite
            tyyppi_ = tyyppiMerkista( pohja.at(0) );
            for(int i=1; i < pohja.length(); i++) {
                if( pohja.at(i) != '0') {
                    kanta_ = pohja.mid(i, pohja.length() - i - 1);
                    break;
                }
            }
        } else {
            tyyppi_ = tyyppiMerkista(pohja.right(1));
            kanta_ = pohja.left( pohja.length() - 1);
        }
    }
}

ViiteNumero::ViiteNumero(ViiteNumero::ViiteNumeroTyyppi tyyppi, qlonglong kanta)
    : ViiteNumero(tyyppi, QString::number(kanta))
{

}

ViiteNumero::ViiteNumero(ViiteNumero::ViiteNumeroTyyppi tyyppi, const QString &kanta) :
    tyyppi_(tyyppi), kanta_(kanta)
{

    if( tyyppi == VAKIOVIITE && kanta.length() < 6) {
        // Lyhyt vakioviite
        viitenumero_ = kanta + laskeTarkaste(kanta);
    } else if(kanta.length() < 5) {
        QString nollat;
        nollat.fill('0', 4 - kanta.length());

        const QString pohja = QString::number(tyyppi) + nollat + kanta + "9";
        viitenumero_ = pohja + laskeTarkaste(pohja);
    } else {
        const QString pohja = kanta + QString::number(tyyppi);
        viitenumero_ = pohja + laskeTarkaste(pohja);
    }
}

QString ViiteNumero::valeilla() const
{
    QString palautettava;
    const int pituus = viitenumero_.length();

    for(int i=0; i < pituus; i++) {
        palautettava.append(viitenumero_.at(i));
        if( (pituus - i - 1) % 5 == 0 && i < pituus - 1)
            palautettava.append(" ");
    }
    return palautettava;
}

QString ViiteNumero::rfviite() const
{
    QString raaka = "RF00" + viitenumero_;
    int tarkiste = 98 - IbanValidator::ibanModulo( raaka );
    QString tarkasteella =  QString("RF%1%2").arg(tarkiste,2,10,QChar('0')).arg(raaka.mid(4));

    QString palautettava;
    for(int i=0; i < tarkasteella.length(); i++)
    {
        palautettava.append(tarkasteella.at(i));
        if( i % 4 == 3)
            palautettava.append(QChar(' '));
    }
    return palautettava;
}

QString ViiteNumero::kanta() const
{
    return kanta_;
}

qlonglong ViiteNumero::numero() const
{
    return kanta().toLongLong();
}

int ViiteNumero::eraId() const
{
    if( tyyppi() == ASIAKAS || tyyppi() == HUONEISTO)
        return 0 - ( kanta().toInt() * 10 + tyyppi());
    return VIRHEELLINEN;
}

QString ViiteNumero::laskeTarkaste(const QString &pohja)
{
    int summa = 0;
    int indeksi = 0;
    for( int i = pohja.length() - 1; i > -1; i--) {
        QChar ch = pohja.at(i);
        int numero = ch.digitValue();

        if( indeksi % 3 == 0)
            summa += 7 * numero;
        else if( indeksi % 3 == 1)
            summa += 3 * numero;
        else
            summa += numero;

        indeksi++;
    }
    return QString::number(( 10 - summa % 10) % 10);
}

ViiteNumero::ViiteNumeroTyyppi ViiteNumero::tyyppiMerkista(const QString &merkki)
{
    if( merkki == "1")
        return LASKU;
    else if( merkki == "2")
        return VAKIOVIITE;
    else if( merkki == "3")
        return ASIAKAS;
    else if( merkki == "4")
        return HUONEISTO;
    else
        return VIRHEELLINEN;
}


