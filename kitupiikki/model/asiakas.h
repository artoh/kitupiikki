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
#ifndef ASIAKAS_H
#define ASIAKAS_H

#include <QObject>
#include <QVariant>

class AsiakasToimittajaTaydentaja;

class Asiakas : public QObject
{
    Q_OBJECT
public:
    explicit Asiakas(QObject *parent = nullptr);

    QString nimi() const { return data_.value("nimi").toString();}
    QString osoite() const { return data_.value("osoite").toString();}
    QString postinumero() const { return data_.value("postinumero").toString();}
    QString kaupunki() const { return data_.value("kaupunki").toString();}
    QString ovt() const { return data_.value("ovt").toString();}
    QString operaattori() const { return data_.value("opetaattori").toString();}

    int id() const { return data_.value("id").toInt();}

    QString ytunnus() const;

    bool muokattu() const { return muokattu_;}

    void set(const QString& avain, const QString& arvo);
    void setYTunnus(QString& tunnus);

    AsiakasToimittajaTaydentaja* taydentaja();

signals:
    void tallennettu();

public slots:
    void lataa(QVariantMap data);
    void valitse(const QString& nimi);
    void clear();
    void tallenna(bool tositteentallennus = false);

private slots:
    void tallennusvalmis(QVariant* var);
    void vaintallennusvalmis(QVariant* var);

protected:
    QVariantMap data_;
    AsiakasToimittajaTaydentaja *taydentaja_ = nullptr;
    bool muokattu_ = false;

};

#endif // ASIAKAS_H
