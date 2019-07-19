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
#ifndef KANTAASIAKASTOIMITTAJA_H
#define KANTAASIAKASTOIMITTAJA_H

#include <QObject>
#include <QVariant>

class AsiakasToimittajaTaydentaja;

class KantaAsiakasToimittaja : public QObject
{
    Q_OBJECT
public:
    explicit KantaAsiakasToimittaja(QObject *parent = nullptr);

    int id() const { return data_.value("id").toInt();}
    QString nimi() const { return data_.value("nimi").toString();}
    QString osoite() const { return data_.value("osoite").toString();}
    QString postinumero() const { return data_.value("postinumero").toString();}
    QString kaupunki() const { return data_.value("kaupunki").toString();}

    QString ytunnus() const;
    QString alvtunnus() const { return data_.value("alvtunnus").toString();}
    QString maa() const;
    QString email() const { return data_.value("email").toString();}

    void set(const QString& avain, const QString& arvo);
    void setYTunnus(const QString &tunnus);

    bool muokattu() const { return muokattu_;}

    virtual AsiakasToimittajaTaydentaja* taydentaja() = 0;

signals:
    void tallennettu();

public slots:

protected slots:
    void tallennusvalmis(QVariant* var);
    void vaintallennusvalmis(QVariant* var);

protected:
    AsiakasToimittajaTaydentaja *taydentaja_ = nullptr;
    QVariantMap data_;
    bool muokattu_ = false;
};

#endif // KANTAASIAKASTOIMITTAJA_H
