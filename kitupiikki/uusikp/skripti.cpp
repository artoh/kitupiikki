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

#include <QRegularExpression>

#include "db/kirjanpito.h"
#include "skripti.h"


Skripti::Skripti()
{

}

void Skripti::suorita()
{
    QRegularExpression tiliRe("^(?<lipo>[+-])(?<mista>\\d+)(..(?<mihin>\\d+))?");
    QRegularExpression asetusRe("^(?<avain>\\w+)(?<lipo>[+-]?)=(?<arvo>.*)");
    asetusRe.setPatternOptions(QRegularExpression::UseUnicodePropertiesOption);

    for( QString rivi : skripti_)
    {
       // Tilikomennolla tili käyttöön taikka piiloon
       QRegularExpressionMatch tiliMats = tiliRe.match(rivi);
       if( tiliMats.hasMatch())
       {
           int mista = tiliMats.captured("mista").toInt();
           int mihin = tiliMats.captured("mihin").toInt();

           int mistaYsi = Tili::ysiluku(mista, false);
           int mihinYsi = 0;

           if( mihin)
               mihinYsi = Tili::ysiluku(mihin, true);
           else
               mihinYsi = Tili::ysiluku(mista, true);


           for(int i = 0; i < kp()->tilit()->rowCount(QModelIndex()); i++)
           {
               QModelIndex index = kp()->tilit()->index(i, 0);
               int ysiluku = index.data(TiliModel::YsiRooli).toInt();
               if( ysiluku >= mistaYsi && ysiluku <= mihinYsi)
               {
                   // Toteutetaan muutos
                   if( tiliMats.captured("lipo") == "+")
                   {
                       if( index.data(TiliModel::TilaRooli) == Tili::TILI_PIILOSSA )
                           kp()->tilit()->setData(index, Tili::TILI_KAYTOSSA, TiliModel::TilaRooli);
                   }
                   else
                   {
                       if( index.data(TiliModel::TilaRooli) != Tili::TILI_PIILOSSA)
                           kp()->tilit()->setData(index, Tili::TILI_PIILOSSA, TiliModel::TilaRooli);
                   }

               }

           }
           continue;
       }

       QRegularExpressionMatch asetusMats = asetusRe.match(rivi);
       if( asetusMats.hasMatch() )
       {
           QString avain = asetusMats.captured("avain");
           QString arvo = asetusMats.captured("arvo");
           QString oper = asetusMats.captured("lipo");

           if( oper == "+")
           {
               QStringList lista = kp()->asetukset()->lista(avain);
               lista.append(arvo);
               kp()->asetukset()->aseta(avain,lista);
           }
           else if( oper == "-")
           {
               QStringList lista = kp()->asetukset()->lista(avain);
               lista.removeAll(arvo);
               kp()->asetukset()->aseta(avain, lista);
           }
           else
           {
               kp()->asetukset()->aseta(avain, arvo);
           }
       }

    }
    kp()->tilit()->tallenna(true);

}

void Skripti::suorita(const QString &skriptinnimi)
{
    Skripti skripti;
    skripti.skripti_ = kp()->asetukset()->lista( skriptinnimi );
    skripti.suorita();
}

void Skripti::suorita(const QStringList &skripti)
{
    Skripti omaSkripti;
    omaSkripti.skripti_ = skripti;
    omaSkripti.suorita();

}
