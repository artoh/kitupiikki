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
#include "euro.h"

Euro::Euro()
{

}

Euro::Euro(qlonglong cents)
    : cents_(cents)
{

}


Euro::Euro(const QString &euroString)
    : cents_( stringToCents(euroString) )
{

}

qlonglong Euro::cents() const
{
    return cents_;
}

double Euro::toDouble() const
{
    return qRound64(cents_ / 100.0);
}

QString Euro::toString() const
{
    const qlonglong euros = cents_ / 100;
    const int extraCents = qAbs(cents_ % 100);

    if( extraCents < 10) {
        return QString::number(euros) + ".0" + QString::number(extraCents);
    } else {
        return QString::number(euros) + "." + QString::number(extraCents);
    }
}

QString Euro::local() const
{
    return QString("%L1").arg( cents_ / 100.0, 0, 'f', 2);
}

QString Euro::display(bool naytaNolla) const
{
    if( !cents_ && !naytaNolla)
        return QString();

    return QString("%L1 €").arg( cents_ / 100.0, 0, 'f', 2);
}

QVariant Euro::toVariant() const
{
    return QVariant( toString() );
}

Euro Euro::operator+(const Euro &other)
{
    qlonglong sum = this->cents() + other.cents();
    return Euro(sum);
}

Euro Euro::operator -(const Euro &other)
{
    qlonglong sub = this->cents() - other.cents();
    return Euro(sub);
}

Euro &Euro::operator+=(const Euro &other)
{
    cents_ += other.cents();
    return *this;
}

Euro &Euro::operator-=(const Euro &other)
{
    cents_ -= other.cents();
    return *this;
}

bool Euro::operator<(const Euro &other)
{
    return this->cents() < other.cents();
}

bool Euro::operator>(const Euro &other)
{
    return this->cents() > other.cents();
}

bool Euro::operator==(const QString& other)
{
    return this->cents() == stringToCents(other);
}

Euro Euro::operator*(const Euro &other)
{
    qlonglong cents = cents_ * other.cents();
    return Euro(cents);
}

Euro Euro::fromVariant(const QVariant &variant)
{
    if( variant.type() == QVariant::String)
        return Euro( variant.toString() );
    else if( variant.type() == QVariant::Double)
        return Euro( qRound64( variant.toDouble() * 100.0) );
    else if( variant.type() == QVariant::LongLong)
        return Euro( variant.toLongLong());
    else {
        qWarning() << "Incorrect variant type " << variant;
        return Euro();
    }
}

Euro Euro::fromDouble(const double euro)
{
    return Euro( qlonglong(euro * 100.0));
}

Euro &Euro::operator<<(const QString &string)
{
    cents_ = stringToCents(string);
    return *this;
}

Euro::operator bool() const
{
    return cents_ != 0l;
}

Euro::operator double() const
{
    return toDouble();
}

Euro::operator QVariant() const
{
    return toVariant();
}

Euro::operator QString() const
{
    return toString();
}

Euro& Euro::operator<<(const QVariant& variant) {
    cents_ = fromVariant(variant).cents();
    return *this;
}


qlonglong Euro::stringToCents(const QString &euroString)
{
    const int decimalIndex = euroString.indexOf('.');
    const int length = euroString.length();
    const int decimals = length - decimalIndex - 1;

    if( decimalIndex == -1) {
        return euroString.toLongLong() * 100;
    }

    const qlonglong euros = euroString.left(decimalIndex).toLongLong();
    const qlonglong extraCents =
            euroString.mid(decimalIndex + 1, 2).toLongLong() *
            (euros < 0 ? -1 : 1);

    if( decimals == 1) {
        return euros * 100 + extraCents * 10;
    }
    return euros * 100 + extraCents;

}

QDataStream& operator<<(QDataStream &out, const Euro& euro) {
    out << euro.toString();
    return out;
}

QDataStream& operator>>(QDataStream &in, Euro &euro) {
    QString euroString;
    in >> euroString;
    euro = Euro(euroString);
    return in;
}

QString& operator<<(QString& out, const Euro& euro) {
    out.append(euro.toString());
    return out;
}

QDebug operator<<(QDebug debug, const Euro& euro) {
    debug.nospace() << euro.display();
    return debug.maybeSpace();
}


bool operator==(const Euro& a, const Euro& b) {
    return a.cents() == b.cents();
}

Euro operator*(const Euro& a, const int b) {
    return Euro( a.cents() * b);
}

Euro operator*(const int a, const Euro& b) {
    return Euro( a * b.cents() );
}

Euro operator*(const Euro& a, const double b) {
    return Euro( qRound64( a.cents() * b) );
}

Euro operator*(const double a, const Euro& b) {
    return Euro( qRound64( a * b.cents() ));
}

Euro operator/(const Euro& a, const int b) {
    return Euro( qRound64( a.cents() * 1.0 / b )  );
}

Euro operator/(const Euro& a, const double b) {
    return Euro( qRound64( a.cents() / b ));
}


