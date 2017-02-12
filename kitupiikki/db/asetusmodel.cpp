/*
   Copyright (C) 2017 Arto Hyvättinen

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

#include <QSqlQuery>

#include <QDate>
#include <QString>
#include <QVariant>

#include "asetusmodel.h"



AsetusModel::AsetusModel(QSqlDatabase *tietokanta, QObject *parent)
    :   QObject(parent), tietokanta_(tietokanta)
{

}

void AsetusModel::aseta(const QString &avain, const QString &arvo)
{
    QSqlQuery query(*tietokanta_);
    if( asetukset_.contains(avain))
    {
        // Asetus on jo, se vain päivitetään
        query.prepare("UPDATE asetus SET arvo=:arvo where avain=:avain");
    }
    else
    {
        // Luodaan uusi asetus
        query.prepare("INSERT INTO asetus(avain,arvo) VALUES(:avain,:arvo)");
    }
    query.bindValue(":avain", avain);
    query.bindValue(":arvo",arvo);
    query.exec();
    asetukset_[avain] = arvo;
}

QDate AsetusModel::pvm(const QString &avain) const
{
    return QDate::fromString( asetus(avain), Qt::ISODate );
}

void AsetusModel::aseta(const QString& avain, const QDate &pvm)
{
    aseta( avain, pvm.toString(Qt::ISODate));
}

bool AsetusModel::onko(const QString &avain) const
{
    if( asetus(avain).isEmpty() || asetus(avain) == "0" || asetus(avain) == "EI")
        return false;
    else
        return true;
}

void AsetusModel::aseta(const QString &avain, bool totuusarvo)
{
    if( totuusarvo )
        aseta( avain, QString("ON"));
    else
        aseta( avain, QString("EI"));
}

void AsetusModel::asetaVar(const QString &avain, const QVariant &arvo)
{
    if( arvo.type() == QVariant::Date)
        aseta( avain, arvo.toDate().toString(Qt::ISODate));
    else if( arvo.type() == QVariant::Bool)
    {
        if( arvo.toBool())
            aseta( avain, true);
        else
            aseta( avain, false );
    }
    else if( arvo.type() == QVariant::StringList)
    {
        aseta( avain, arvo.toStringList().join('\n'));
    }
    else
        aseta( avain, arvo.toString());
}

QStringList AsetusModel::lista(const QString &avain) const
{
    return asetus(avain).split('\n');
}

void AsetusModel::aseta(const QString &avain, const QStringList &arvo)
{
    aseta( avain, arvo.join('\n'));
}

int AsetusModel::luku(const QString &avain) const
{
    return asetus(avain).toInt();
}

void AsetusModel::aseta(const QString& avain, int luku)
{
    aseta(avain, QString::number(luku));
}

void AsetusModel::lataa()
{
    QSqlQuery query(*tietokanta_);
    query.exec("SELECT avain,arvo FROM asetus");
    while( query.next())
    {
        asetukset_[query.value(0).toString()] = query.value(1).toString();
    }

}
