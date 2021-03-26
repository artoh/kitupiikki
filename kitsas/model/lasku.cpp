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
#include "lasku.h"
#include "tositerivit.h"
#include "laskutus/viitenumero.h"

Lasku::Lasku()
    : KantaVariantti()
{

}

Lasku::Lasku(const QVariantMap &data) :
    KantaVariantti(data)
{

}

void Lasku::kopioi(const Lasku &lasku)
{
    data_ = lasku.data();
}

QString Lasku::virtuaaliviivakoodi(const Iban &iban, bool rf) const
{
    qlonglong sentit = summa().cents();
    ViiteNumero viitenumero(viite());

    if( sentit <= 0 || sentit > 99999999
            || viitenumero.tyyppi() == ViiteNumero::VIRHEELLINEN) return QString();

    QString koodi = rf ?
                QString("5 %1 %2 %3 %4 %5")
                       .arg(iban.valeitta().mid(2,16))    // Numeerinen tilinumero
                       .arg(sentit, 8, 10, QChar('0'))
                       .arg(viitenumero.rfviite().mid(2,2) )
                       .arg(viitenumero.rfviite().remove(' ').mid(4),21,QChar('0'))
                       .arg( erapvm().isValid() ? erapvm().toString("yyMMdd") : "000000")
              :
              QString("4 %1 %2 000 %3 %4")
                .arg( iban.valeitta().mid(2,16) )  // Tilinumeron numeerinen osuus
                .arg( sentit, 8, 10, QChar('0') )  // Rahamäärä
                .arg( viitenumero.viite(), 20, QChar('0'))
                .arg( erapvm().isValid() ? erapvm().toString("yyMMdd") : "000000");

    return koodi.remove(QChar(' '));
}

QString Lasku::QRkooditieto(const Iban &iban, const QString &nimi, bool rf) const
{
    // Esitettävä tieto
    QString data("BCD\n001\n1\nSCT\n");

    Iban tilinumero(iban);

    QString bic = tilinumero.bic();
    if( bic.isEmpty())
        return QByteArray();
    data.append(bic + "\n");
    data.append(nimi + "\n");
    data.append(iban.valeitta() + "\n");
    data.append( QString("EUR%1\n\n").arg( summa().toString() ));
    data.append(( rf ? viite().rfviite() : viite().viite() ).remove(QChar(' ')) + "\n\n");
    if( erapvm().isValid())
        data.append( QString("ReqdExctnDt/%1").arg( erapvm().toString(Qt::ISODate) ));
    return data;
}

QDate Lasku::oikaiseErapaiva(QDate erapvm)
{
    while( erapvm.dayOfWeek() > 5 ||
           (erapvm.day()==1 && erapvm.month()==1) ||
           (erapvm.day()==6 && erapvm.month()==1) ||
           (erapvm.day()==1 && erapvm.month()==5) ||
           (erapvm.day()==6 && erapvm.month()==12) ||
           (erapvm.day()>= 24 && erapvm.day() <= 26 && erapvm.month()==12))
        erapvm = erapvm.addDays(1);
    return erapvm;
}
