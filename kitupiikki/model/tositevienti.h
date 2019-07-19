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
        ERAID
    };


    TositeVienti(const QVariantMap& vienti = QVariantMap());

    QVariant data(int kentta) const;
    void set(int kentta, const QVariant& arvo);

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
    int eraId() const { return data(ERAID).toInt();}
    QList<int> merkkaukset() const;

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

private:
    static std::map<int,QString> avaimet__;

};

#endif // TOSITEVIENTI_H
