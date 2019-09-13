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
#ifndef KANTAVARIANTTI_H
#define KANTAVARIANTTI_H

#include <QVariant>

/**
 * @brief QVariant-pohjaisten luokkien kantaluokka
 */
class KantaVariantti
{
public:
    KantaVariantti(const QVariantMap& data = QVariantMap());

    QVariant arvo(const QString& avain) const;
    QString str(const QString& avain) const;
    int luku(const QString& avain) const;
    double dbl(const QString& avain) const;
    QDate pvm(const QString& avain) const;
    void clear();
    void set(const QString& avain, const QVariant& arvo);
    void unset(const QString& avain);

    QVariantMap data() const { return data_;}

protected:
     QVariantMap data_;
};

#endif // KANTAVARIANTTI_H
