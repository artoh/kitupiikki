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
#ifndef TOSITEVIENTI_H
#define TOSITEVIENTI_H

#include <QVariant>
#include <QDate>
#include <map>

class TositeVienti : public QVariantMap
{
public:
    enum Avain {
        ID,
        PVM,
        TILI,
        DEBET,
        KREDIT,
        SELITE,
        ALVKOODI,
        ALVPROSENTTI,
        KOHDENNUS,
        MERKKAUKSET,
        JAKSOALKAA,
        JAKSOLOPPUU,
        ERAID,
        ARKISTOTUNNUS,
        VIITE,
        ERAPAIVA,
        KUMPPANI,
        TYYPPI,
        PALKKAKOODI,
        TASAERAPOISTO
    };

    enum VientiTyyppi {
        KIRJAUS = 1,
        VASTAKIRJAUS = 2,
        ALVKIRJAUS = 3,
        MAAHANTUONTIVASTAKIRJAUS = 31,
        OSTO = 100,
        MYYNTI = 200,
        SUORITUS = 300,
        BRUTTOOIKAISU = 91091,
        POISTO = 99100,
        JAKSOTUS_TP = 99210,
        JAKSOTUS_TA = 99220

    };


    TositeVienti(const QVariantMap& vienti = QVariantMap());

    QVariant data(int kentta) const;
    void set(int kentta, const QVariant& arvo);
    QVariant tallennettava() const;

    int id() const { return data(ID).toInt();}
    QDate pvm() const { return  data(PVM).toDate();}
    int alvkoodi() const { return data(ALVKOODI).toInt();}
    int tili() const { return data(TILI).toInt();}
    double debet() const { return data(DEBET).toDouble();}
    double kredit() const { return data(KREDIT).toDouble();}
    int alvKoodi() const { return data(ALVKOODI).toInt();}
    double alvProsentti() const { return data(ALVPROSENTTI).toDouble();}
    int kohdennus() const { return  data(KOHDENNUS).toInt();}
    QString selite() const { return data(SELITE).toString();}
    int eraId() const;
    int kumppaniId() const;
    QList<int> merkkaukset() const;
    QString arkistotunnus() const { return data(ARKISTOTUNNUS).toString();}
    QString viite() const { return data(VIITE).toString();}
    QDate erapaiva() const { return data(ERAPAIVA).toDate();}
    int tyyppi() const { return data(TYYPPI).toInt(); }
    QString palkkakoodi() const { return data(PALKKAKOODI).toString(); }
    int tasaerapoisto() const { return data(TASAERAPOISTO).toInt();}
    QDate jaksoalkaa() const { return data(JAKSOALKAA).toDate();}
    QDate jaksoloppuu() const { return data(JAKSOLOPPUU).toDate();}

    void setPvm(const QDate& pvm);
    void setTili(int tili);
    void setDebet(double euroa);
    void setDebet(qlonglong senttia);
    void setKredit(double euroa);
    void setKredit(qlonglong senttia);
    void setSelite(const QString& selite);
    void setAlvKoodi(int koodi);
    void setAlvProsentti(double prosentti);
    void setKohdennus(int kohdennus);
    void setMerkkaukset( QVariantList merkkaukset);
    void setJaksoalkaa( const QDate& pvm);
    void setJaksoloppuu( const QDate& pvm );
    void setEra(int era);
    void setArkistotunnus(const QString& tunnus);
    void setViite(const QString& viite);
    void setErapaiva(const QDate& erapvm);
    void setKumppani(int kumppaniId);
    void setTyyppi(int tyyppi);
    void setPalkkakoodi(const QString& palkkakoodi);
    void setTasaerapoisto(int kuukautta);

private:
    static std::map<int,QString> avaimet__;

};

#endif // TOSITEVIENTI_H
