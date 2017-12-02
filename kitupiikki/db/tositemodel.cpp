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

TositeModel::TositeModel(QSqlDatabase *tietokanta, QObject *parent)
    : QObject(parent), tietokanta_(tietokanta), muokattu_(false)
{
    vientiModel_ = new VientiModel(this);
    liiteModel_ = new LiiteModel(this);
    tyhjaa();
}

Tositelaji TositeModel::tositelaji() const
{
    return kp()->tositelajit()->tositelaji( tositelaji_ );
}

bool TositeModel::muokkausSallittu() const
{
    // Jos järjestelmätosite tai päätetyllä tilikaudella, niin
    // ei saa muokata
    if( tositelaji().id() == 0 || kp()->tilitpaatetty().daysTo( pvm () ) < 0)
        return false;

    return true;
}

int TositeModel::seuraavaTunnistenumero() const
{
    return tositelaji().seuraavanTunnistenumero( pvm() );
}

bool TositeModel::kelpaakoTunniste(int tunnistenumero) const
{
    // Tunniste ei kelpaa, jos kyseisellä kaudella se on jo

    Tilikausi kausi = kp()->tilikausiPaivalle( pvm() );
    QString kysymys = QString("SELECT id FROM tosite WHERE tunniste=\"%1\" "
                              "AND pvm BETWEEN \"%2\" AND \"%3\" AND id <> %4 "
                              "AND laji=\"%5\" ").arg( tunnistenumero )
                                                 .arg(kausi.alkaa().toString(Qt::ISODate))
                                                 .arg(kausi.paattyy().toString(Qt::ISODate))
                                                 .arg( id() )
                                                 .arg( tositelaji_ );
    QSqlQuery kysely( *tietokanta_ );
    kysely.exec(kysymys);
    return !kysely.next();
}

bool TositeModel::muokattu()
{
    return muokattu_ || vientiModel()->muokattu() || json()->muokattu() || liiteModel()->muokattu();
}

void TositeModel::asetaPvm(const QDate &pvm)
{
    if( pvm != pvm_)
    {
        pvm_ = pvm;
        muokattu_ = true;
    }
}

void TositeModel::asetaOtsikko(const QString &otsikko)
{
    if( otsikko != otsikko_)
    {
        otsikko_ = otsikko;
        muokattu_ = true;
    }
}

void TositeModel::asetaKommentti(const QString &kommentti)
{
    if( kommentti != kommentti_)
    {
        kommentti_ = kommentti;
        muokattu_ = true;
    }
}

void TositeModel::asetaTunniste(int tunniste)
{
    if( tunniste != tunniste_)
    {
        tunniste_ = tunniste;
        muokattu_ = true;
    }
}


void TositeModel::asetaTositelaji(int tositelajiId)
{
    if( tositelajiId != tositelaji_)
    {
        tositelaji_ = tositelajiId;
        // Vaihdetaan sopiva tunniste
        tunniste_ = seuraavaTunnistenumero();

        muokattu_ = true;
    }

}

void TositeModel::asetaTiliotetili(int tiliId)
{
    if( tiliId != tiliotetili_)
    {
        tiliotetili_ = tiliId;
        muokattu_ = true;
    }
}

void TositeModel::lataa(int id)
{
    // Lataa tositteen

    QSqlQuery kysely(*tietokanta_);
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
    muokattu_ = false;
}

void TositeModel::tyhjaa()
{
    // Tyhjentää tositteen
    id_ = 0;
    pvm_ = kp()->paivamaara();
    otsikko_ = QString();
    kommentti_ = QString();
    tositelaji_ = 1;
    tunniste_ = seuraavaTunnistenumero();
    tiliotetili_ = 0;

    vientiModel_->tyhjaa();
    liiteModel_->tyhjaa();
    muokattu_ = false;

}

void TositeModel::tallenna()
{
    // Tallentaa tositteen


    QSqlQuery kysely(*tietokanta_);
    if( id() )
    {
        kysely.prepare("UPDATE tosite SET pvm=:pvm, otsikko=:otsikko, kommentti=:kommentti, "
                       "tunniste=:tunniste, laji=:laji, tiliote=:tiliote, json=:json WHERE id=:id");
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

    kysely.bindValue(":json", json_.toSqlJson());


    kysely.exec();


    if( !id())
        id_ = kysely.lastInsertId().toInt();

    vientiModel_->tallenna();
    liiteModel_->tallenna();

    emit kp()->kirjanpitoaMuokattu();
    muokattu_ = false;
}
