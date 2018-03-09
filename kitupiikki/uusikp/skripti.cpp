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


Skripti::Skripti() :
    asetusModel_( kp()->asetukset() ), tiliModel_( kp()->tilit() )
{

}

Skripti::Skripti(AsetusModel *asetusModel, TiliModel *tiliModel) :
    asetusModel_(asetusModel), tiliModel_(tiliModel)
{

}

void Skripti::suorita()
{
    QRegularExpression tiliRe("^(?<lipo>[-+*])?(?<mista>\\d+)(/(?<taso>\\d+))?(..(?<mihin>\\d+))?(\\s(?<avain>\\w+)=(?<arvo>.+))?");
    QRegularExpression asetusRe("^(?<avain>\\w+)(?<lipo>[-+]?)=(?<arvo>.*)");
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
           if( tiliMats.captured("taso").toInt())
               mistaYsi += tiliMats.captured("taso").toInt()-1;
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
                   else if(tiliMats.captured("lipo") == "-")
                   {
                       if( index.data(TiliModel::TilaRooli) != Tili::TILI_PIILOSSA)
                           kp()->tilit()->setData(index, Tili::TILI_PIILOSSA, TiliModel::TilaRooli);
                   }
                   else if(tiliMats.captured("lipo") == "*")
                   {
                       if( index.data(TiliModel::TilaRooli) != Tili::TILI_SUOSIKKI)
                           kp()->tilit()->setData(index, Tili::TILI_SUOSIKKI, TiliModel::TilaRooli);
                   }

                   QString avain = tiliMats.captured("avain");
                   if( !avain.isEmpty() && index.data(TiliModel::OtsikkotasoRooli).toInt() == 0)
                   {
                       QString arvo = tiliMats.captured("arvo");
                       if( avain == "T")
                           kp()->tilit()->setData(index, arvo, TiliModel::TyyppiRooli );
                       else
                       {
                            if( arvo.toInt())
                                kp()->tilit()->jsonIndeksilla(i)->set(avain, arvo.toInt());
                            else
                                kp()->tilit()->jsonIndeksilla(i)->set(avain, arvo);
                       }
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

void Skripti::suorita(const QStringList &skripti, AsetusModel *asetusModel, TiliModel *tiliModel)
{
    Skripti omaSkripti(asetusModel, tiliModel);
    omaSkripti.skripti_ = skripti;
    omaSkripti.suorita();
}

void Skripti::suorita(const QStringList &skripti)
{
    Skripti omaSkripti;
    omaSkripti.skripti_ = skripti;
    omaSkripti.suorita();

}
