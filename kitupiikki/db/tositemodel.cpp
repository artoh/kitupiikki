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

#include "tositemodel.h"

#include "db/tositelajimodel.h"
#include "db/kirjanpito.h"

#include <QDebug>
#include <QSqlError>

TositeModel::TositeModel(QSqlDatabase tietokanta, QObject *parent)
    : QObject(parent), tietokanta_(tietokanta)
{
    vientiModel_ = new VientiModel(this);
    liiteModel_ = new LiiteModel(this);
    tyhjaa();
}

Tositelaji TositeModel::tositelaji() const
{
    return kp()->tositelajit()->tositelaji( tositelaji_ );
}

void TositeModel::asetaPvm(const QDate &pvm)
{
    pvm_ = pvm;
}

void TositeModel::asetaOtsikko(const QString &otsikko)
{
    otsikko_ = otsikko;
}

void TositeModel::asetaKommentti(const QString &kommentti)
{
    kommentti_ = kommentti;
}

void TositeModel::asetaTunniste(int tunniste)
{
    tunniste_ = tunniste;
    // TODO: Tarkistukset
}


void TositeModel::asetaTositelaji(int tositelajiId)
{
    tositelaji_ = tositelajiId;
    // TODO: Tarkistukset
}

void TositeModel::asetaTiliotetili(int tiliId)
{
    tiliotetili_ = tiliId;
}

void TositeModel::lataa(int id)
{
    // Lataa tositteen

    QSqlQuery kysely(tietokanta_);
    kysely.exec( QString("SELECT pvm, otsikko, kommentti, tunniste,"
                              "laji, tiliote, json FROM tosite "
                              "WHERE id = %1").arg(id));
    if( kysely.next())
    {
        id_ = id;
        otsikko_ = kysely.value("otsikko").toString();
        kommentti_ = kysely.value("kommentti").toString();
        tunniste_ = kysely.value("tunniste").toInt();
        tositelaji_ = kysely.value("laji").toInt();
        tiliotetili_ = kysely.value("tiliote").toInt();
        json_.fromJson( kysely.value("json").toByteArray());

        vientiModel_->lataa();
        liiteModel_->lataa();
    }
}

void TositeModel::tyhjaa()
{
    // Tyhjentää tositteen
    id_ = 0;
    pvm_ = kp()->paivamaara();
    otsikko_ = QString();
    kommentti_ = QString();
    tunniste_ = 0;
    tositelaji_ = 1;
    tiliotetili_ = 0;

    vientiModel_->tyhjaa();
    liiteModel_->tyhjaa();
}

void TositeModel::tallenna()
{
    // Tallentaa tositteen


    QSqlQuery kysely(tietokanta_);
    if( id() )
    {
        kysely.prepare("UPDATE tosite SET pvm=:pvm, otsikko=:otsikko, kommentti:=kommentti,"
                       "tunniste=:tunniste, laji=:laji, tiliote=:tiliote, json:=json WHERE id=:id");
        kysely.bindValue(":id", id());
    }
    else
    {
        kysely.prepare("INSERT INTO tosite(pvm, otsikko, kommentti, tunniste, laji, tiliote, json) "
                       "VALUES(:pvm, :otsikko, :kommentti, :tunniste, :laji, :tiliote, :json)");
    }
    kysely.bindValue(":pvm", pvm());
    kysely.bindValue(":otsikko", otsikko());
    if( kommentti().isEmpty())
        kysely.bindValue(":kommentti", QVariant() );
    else
        kysely.bindValue(":kommentti", kommentti());
    kysely.bindValue(":tunniste", tunniste());
    kysely.bindValue(":laji", tositelaji().id());

    if( tiliotetili())
        kysely.bindValue(":tiliote", tiliotetili());
    else
        kysely.bindValue(":tiliote", QVariant());

    kysely.bindValue(":json", json_.toJson());


    kysely.exec();

    qDebug() << kysely.lastQuery() << " " << kysely.lastError().text();

    if( !id())
        id_ = kysely.lastInsertId().toInt();

    vientiModel_->tallenna();
    liiteModel_->tallenna();
}
