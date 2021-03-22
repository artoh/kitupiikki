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
#ifndef TUOTE_H
#define TUOTE_H

#include <QVariantMap>

class Tuote
{
public:
    Tuote();
    Tuote(const QVariantMap& map);

    QVariantMap toMap() const;

    int id() const;
    void setId(int id);

    QString nimike() const;
    void setNimike(const QString &nimike);

    QString yksikko() const;
    void setYksikko(const QString &yksikko);

    QString unKoodi() const;
    void setUnKoodi(const QString &unKoodi);

    double ahinta() const;
    void setAhinta(double ahinta);

    int kohdennus() const;
    void setKohdennus(int kohdennus);

    int tili() const;
    void setTili(int tili);

    int alvkoodi() const;
    void setAlvkoodi(int alvkoodi);

    double alvprosentti() const;
    void setAlvprosentti(double alvprosentti);

private:
    int id_ = 0;
    QString nimike_;
    QString yksikko_;
    QString unKoodi_;
    double ahinta_;
    int kohdennus_;
    int tili_;
    int alvkoodi_;
    double alvprosentti_;
};

#endif // TUOTE_H
