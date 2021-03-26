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
#ifndef EURO_H
#define EURO_H

#include <QtGlobal>
#include <QMetaType>
#include <QString>
#include <QDataStream>
#include <QVariant>
#include <QDebug>


class Euro
{
public:
    Euro();
    Euro(qlonglong cents);
    Euro(const QString& euroString);

    qlonglong cents() const;
    double toDouble() const;
    QString toString() const;
    QString local() const;
    QString display(bool naytaNolla = true) const;
    QVariant toVariant() const;

    Euro operator+(const Euro& other);
    Euro operator-(const Euro& other);

    Euro& operator+=(const Euro& other);
    Euro& operator-=(const Euro& other);

    bool operator<(const Euro& other);
    bool operator>(const Euro& other);

    bool operator==(const QString& other);

    Euro operator*(const Euro& other);

    static Euro fromVariant(const QVariant& variant);
    static Euro fromDouble(const double euro);

    Euro& operator<<(const QString& string);
    Euro& operator<<(const QVariant& variant);

    operator QString() const;
    operator QVariant() const;
    operator double() const;
    operator bool() const;

private:
    static qlonglong stringToCents(const QString& euroString);


private:
    qlonglong cents_ = 0l;
};

Q_DECLARE_METATYPE(Euro);

bool operator==(const Euro& a, const Euro& b);
Euro operator*(const Euro& a, const int b);
Euro operator*(const int a, const Euro& b);
Euro operator*(const Euro& a, const double b);
Euro operator*(const double a, const Euro& b);

Euro operator/(const Euro& a, const int b);
Euro operator/(const Euro& a, const float b);

QDataStream& operator<<(QDataStream &out, const Euro &euro);
QDataStream& operator>>(QDataStream &in, Euro &euro);

QString& operator<<(QString& out, const Euro& euro);
QDebug operator<<(QDebug debug, const Euro& euro);

#endif // EURO_H
