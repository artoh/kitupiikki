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
#include <QMessageBox>

TositeModel::TositeModel(QSqlDatabase *tietokanta, QObject *parent)
    : QObject(parent),
      id_(-1), pvm_(kp()->paivamaara()), tositelaji_(1), tiliotetili_(0),
      tietokanta_(tietokanta),
      muokattu_(false)
{
    vientiModel_ = new VientiModel(this);
    liiteModel_ = new LiiteModel(this);
    tunniste_ = seuraavaTunnistenumero();

    connect( vientiModel(), &VientiModel::muuttunut, [this] { emit tositettaMuokattu(true); } );
    connect( liiteModel(), &LiiteModel::liiteMuutettu, [this] { emit tositettaMuokattu(true);} );
}

Tositelaji TositeModel::tositelaji() const
{
    return kp()->tositelajit()->tositelaji( tositelaji_ );
}

bool TositeModel::muokkausSallittu() const
{
    // Jos päätetyllä tilikaudella, niin
    // ei saa muokata
    return pvm() >= kp()->tilitpaatetty() &&
           kp()->tilitpaatetty() < kp()->tilikaudet()->kirjanpitoLoppuu();
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

    return muokattu_ || vientiModel()->muokattu() || json()->onkoMuokattu() || liiteModel()->muokattu();
}

void TositeModel::asetaPvm(const QDate &pvm)
{
    if( pvm == pvm_ )
        return;

   pvm_ = pvm;

   if( id_ > -1)
   {
        // Pelkkä uuden tositteen päivämäärän muuttaminen
        // ei tarkoita tositteen muokkaamista
        muokattu_ = true;
        emit tositettaMuokattu(true);
   }

}

void TositeModel::asetaOtsikko(const QString &otsikko)
{
    if( otsikko != otsikko_)
    {
        otsikko_ = otsikko;
        muokattu_ = true;
        emit tositettaMuokattu(true);
    }
}

void TositeModel::asetaKommentti(const QString &kommentti)
{
    if( kommentti != kommentti_)
    {
        kommentti_ = kommentti;
        muokattu_ = true;
        emit tositettaMuokattu(true);
    }
}

void TositeModel::asetaTunniste(int tunniste)
{
    if( tunniste != tunniste_)
    {
        tunniste_ = tunniste;
        muokattu_ = true;
        emit tositettaMuokattu(true);
    }
}

void TositeModel::asetaTositelaji(int tositelajiId)
{
    if( tositelajiId != tositelaji_)
    {
        tositelaji_ = tositelajiId;
        // Vaihdetaan sopiva tunniste
        tunniste_ = seuraavaTunnistenumero();

        // Pelkkä tositelajin muutos ei merkitse uutta
        // tositetta muokatuksi
        if( id_ > -1)
        {
            muokattu_ = true;
            emit tositettaMuokattu(true);
        }
    }

}

void TositeModel::asetaTiliotetili(int tiliId)
{
    if( tiliId != tiliotetili_)
    {
        tiliotetili_ = tiliId;
        muokattu_ = true;
        emit tositettaMuokattu(true);
    }
}

void TositeModel::lataa(int id)
{
    // Lataa tositteen

    QSqlQuery kysely(*tietokanta_);
    kysely.exec( QString("SELECT pvm, otsikko, kommentti, tunniste,"
                              "laji, tiliote, json, luotu, muokattu FROM tosite "
                              "WHERE id = %1").arg(id));
    if( kysely.next())
    {
        id_ = id;
        pvm_ = kysely.value("pvm").toDate();
        otsikko_ = kysely.value("otsikko").toString();
        kommentti_ = kysely.value("kommentti").toString();
        tunniste_ = kysely.value("tunniste").toInt();
        tositelaji_ = kysely.value("laji").toInt();
        tiliotetili_ = kysely.value("tiliote").toInt();
        json_.fromJson( kysely.value("json").toByteArray());
        luotu_ = kysely.value("luotu").toDateTime();
        muokattuAika_ = kysely.value("muokattu").toDateTime();

        vientiModel_->lataa();
        liiteModel_->lataa();
    }
    muokattu_ = false;

    emit tositettaMuokattu(false);
    emit tyhjennetty();
}

void TositeModel::tyhjaa()
{
    // Tyhjentää tositteen
    id_ = -1;

    // Siltä varalta että kuluva tilikausi on jo lukittu, siirtyy seuraavaan sallittuun päivään
    if( pvm_ <= kp()->tilitpaatetty() )
    {
        if( kp()->tilikaudet()->kirjanpitoLoppuu() > kp()->tilitpaatetty() )
            pvm_ = kp()->tilitpaatetty().addDays(1);
    }

    otsikko_ = QString();
    kommentti_ = QString();
    tunniste_ = seuraavaTunnistenumero();
    tiliotetili_ = 0;

    vientiModel_->tyhjaa();
    liiteModel_->tyhjaa();
    muokattu_ = false;

    emit tositettaMuokattu(false);
    emit tyhjennetty();

}

bool TositeModel::tallenna()
{
    // Tallentaa tositteen
    tietokanta()->transaction();

    QSqlQuery kysely(*tietokanta_);
    if( id() > -1)
    {
        kysely.prepare("UPDATE tosite SET pvm=:pvm, otsikko=:otsikko, kommentti=:kommentti, "
                       "tunniste=:tunniste, laji=:laji, tiliote=:tiliote, json=:json, muokattu=:muokattu WHERE id=:id");
        kysely.bindValue(":id", id());
    }
    else
    {
        kysely.prepare("INSERT INTO tosite(pvm, otsikko, kommentti, tunniste, laji, tiliote, json, luotu, muokattu) "
                       "VALUES(:pvm, :otsikko, :kommentti, :tunniste, :laji, :tiliote, :json, :luotu, :muokattu)");

        kysely.bindValue(":luotu", QDateTime::currentDateTime());
        luotu_ = QDateTime::currentDateTime();
    }
    kysely.bindValue(":pvm", pvm());
    kysely.bindValue(":otsikko", otsikko());
    if( kommentti().isEmpty())
        kysely.bindValue(":kommentti", QVariant() );
    else
        kysely.bindValue(":kommentti", kommentti());

    if( tunniste() )
        kysely.bindValue(":tunniste", tunniste());
    else
        kysely.bindValue(":tunniste", QVariant());

    kysely.bindValue(":laji", tositelaji().id());

    if( tiliotetili())
        kysely.bindValue(":tiliote", tiliotetili());
    else
        kysely.bindValue(":tiliote", QVariant());

    kysely.bindValue(":json", json_.toSqlJson());
    kysely.bindValue(":muokattu", QDateTime::currentDateTime());

    if( !kysely.exec() )
    {
        tietokanta()->rollback();
        qDebug() << kysely.lastQuery() << kysely.lastError().text();
        return false;
    }


    if( id() < 0)
        id_ = kysely.lastInsertId().toInt();

    if( !vientiModel_->tallenna() || !liiteModel_->tallenna() )
    {
        // Tallennuksessa virheitä, perutaan ja palautetaan virhe
        qDebug() << tietokanta()->lastError().text();
        tietokanta()->rollback();
        return false;
    }

    tietokanta()->commit();

    emit kp()->kirjanpitoaMuokattu();
    muokattu_ = false;
    muokattuAika_ = QDateTime::currentDateTime();

    emit tositettaMuokattu(false);

    return true;
}

bool TositeModel::poista()
{
    // Tosite on poistettava ihan oikeasti, koska muuten sotkee tase-erät
    // Liitteiden tiedostoja ei kuitenkaan poisteta

    if( !id())
        return true;

    tietokanta()->transaction();
    QSqlQuery kysely(*tietokanta());

    kysely.exec(QString("DELETE FROM vienti WHERE tosite=%1").arg( id() ));
    kysely.exec(QString("DELETE FROM liite WHERE tosite=%1").arg( id() ));
    kysely.exec(QString("DELETE FROM tosite WHERE id=%1").arg( id()) );

    if( tietokanta()->commit())
    {
        emit kp()->kirjanpitoaMuokattu();
        return true;
    }
    else
        return false;


}

void TositeModel::uusiPohjalta(const QDate &pvm, const QString &otsikko)
{
    json_.set("KopioituTositteelta", id_);

    id_ = -1;   // Ei tallennettu

    luotu_ = QDateTime();
    muokattuAika_ = QDateTime();

    // Asetetaan päivämäärä ja haetaan uusi tunnistenumero
    pvm_ = pvm;
    tunniste_ = seuraavaTunnistenumero();

    // Viennit
    vientiModel()->uusiPohjalta(otsikko);

    otsikko_ = otsikko;

    liiteModel_->tyhjaa();

    muokattu_ = true;

}

RaportinKirjoittaja TositeModel::tuloste()
{
    RaportinKirjoittaja rk;
    rk.asetaOtsikko("TOSITE");

    rk.lisaaPvmSarake();
    rk.lisaaSarake("999999 Tilin nimi tarkenne");
    rk.lisaaSarake("Kohdennustaa");
    rk.lisaaVenyvaSarake();
    rk.lisaaEurosarake();
    rk.lisaaEurosarake();

    rk.lisaaRivi();

    RaporttiRivi orivi;
    orivi.lisaa( pvm().toString("dd.MM.yyyy"), 2 );
    orivi.lisaa( otsikko(), 2);
    orivi.lisaa( QString("%1%2/%3")
                   .arg(tositelaji().tunnus()).arg(tunniste()).arg( kp()->tilikaudet()->tilikausiPaivalle( pvm() ).kausitunnus()),2,true);
    orivi.asetaKoko(16);
    rk.lisaaRivi(orivi);

    rk.lisaaRivi();

    // Sitten viennit
    int vienteja = vientiModel()->rowCount(QModelIndex());
    if( vienteja )
    {
        RaporttiRivi th;
        th.lisaa(tr("Pvm"));
        th.lisaa(tr("Tili"));
        th.lisaa(tr("Kohdennus"));
        th.lisaa(tr("Selite"));
        th.lisaa(tr("Debet"));
        th.lisaa(tr("Kredit"));
        rk.lisaaRivi(th);

        for(int vientiRivi = 0; vientiRivi < vienteja; vientiRivi++)
        {
            QModelIndex index = vientiModel()->index(vientiRivi,0);
            RaporttiRivi rr;

            rr.lisaa( index.data(VientiModel::PvmRooli).toDate() );
            rr.lisaa( index.sibling(vientiRivi, VientiModel::TILI).data().toString());
            rr.lisaa( index.sibling(vientiRivi, VientiModel::KOHDENNUS).data().toString() );
            rr.lisaa( index.sibling(vientiRivi, VientiModel::SELITE).data().toString() );
            rr.lisaa( index.data(VientiModel::DebetRooli).toLongLong());
            rr.lisaa( index.data(VientiModel::KreditRooli).toLongLong());

            if( vientiRivi == 0)
                rr.viivaYlle();
            rk.lisaaRivi(rr);
        }
        RaporttiRivi vr;
        vr.viivaYlle();
        rk.lisaaRivi(vr);
    }

    // Kommentit
    if( !kommentti().isEmpty())
    {
        RaporttiRivi rr;
        rr.lisaa( kommentti(), 6);
        rk.lisaaRivi(rr);
        rk.lisaaRivi();
    }

    // Liitteet
    if( liiteModel()->rowCount(QModelIndex()))
    {
        RaporttiRivi rr;
        rr.lisaa("Liitteet",2);
        rr.lihavoi();
        rk.lisaaRivi(rr);

        for(int liite=0; liite < liiteModel()->rowCount(QModelIndex()); liite++)
        {
            RaporttiRivi lr;
            QModelIndex lInd = liiteModel()->index(liite,0);
            lr.lisaa(lInd.data(LiiteModel::OtsikkoRooli ).toString(), 6);
            rk.lisaaRivi(lr);
        }
    }

    return rk;
}

