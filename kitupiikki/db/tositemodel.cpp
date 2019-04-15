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
#include <QSqlRecord>

#include <QJsonDocument>

#include "aloitussivu/aloitussivu.h"
#include "versio.h"


TositeModel::TositeModel(QSqlDatabase *tietokanta, QObject *parent)
    : QAbstractTableModel (parent),
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

int TositeModel::rowCount(const QModelIndex &parent) const
{
    return viennit_.count();
}

int TositeModel::columnCount(const QModelIndex &parent) const
{
    return 7;
}

QVariant TositeModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if( role == Qt::TextAlignmentRole)
        return QVariant( Qt::AlignCenter | Qt::AlignVCenter);
    else if( role != Qt::DisplayRole )
        return QVariant();
    else if( orientation == Qt::Horizontal)
    {
        switch (section)
        {
            case PVM:
                return QVariant("Pvm");
            case TILI:
                return QVariant("Tili");
            case DEBET :
                return QVariant("Debet");
            case KREDIT:
                return QVariant("Kredit");
            case ALV:
                return QVariant("Alv");
            case SELITE:
                return QVariant("Selite");
            case KOHDENNUS :
                return QVariant("Kohdennus");

        }

    }
    return QVariant( section + 1);
}

QVariant TositeModel::data(const QModelIndex &index, int role) const
{
    QVariantMap vienti = viennit_.at(index.row());

    if( role == Qt::DisplayRole )
    {
        switch ( index.column()) {
        case PVM:
            return vienti.value("pvm").toDate();
        case TILI:
        {
            Tili *tili = kp()->tilit()->tiliNumerolla( vienti.value("tili").toInt() );
            if( tili )
                return QString("%1 %2").arg(tili->numero()).arg(tili->nimi());
            return QVariant();
        }
        case DEBET:
        {
            double debet = vienti.value("debet").toDouble();
             if( debet > 1e-5 )
                return QVariant( QString("%L1 €").arg(debet,0,'f',2));
             return QVariant();
        }
        case KREDIT:
        {
            double kredit = vienti.value("kredit").toDouble();
            if( kredit > 1e-5)
                return QVariant( QString("%L1 €").arg(kredit,0,'f',2));
             return QVariant();
        }
        case ALV:
        {
            int alvkoodi = vienti.value("alvkoodi").toInt();
            if( alvkoodi == AlvKoodi::EIALV )
                return QVariant();
            else
            {
                if( alvkoodi == AlvKoodi::MAKSETTAVAALV)
                    return tr("VERO");
                else if(alvkoodi == AlvKoodi::TILITYS)
                    return QString();
                else
                    return QVariant( QString("%1 %").arg( vienti.value("alvprosentti").toInt() ));
            }
        }
        case SELITE:
            return vienti.value("selite");
        case KOHDENNUS:
            return QVariant();

        }

    }
    else if( role == Qt::EditRole )
    {
        switch ( index.column())
        {
        case PVM:
            return vienti.value("pvm").toDate();
        case TILI:
            return vienti.value("tili").toInt();
        case DEBET:
            return vienti.value("debetsnt").toLongLong() / 100.0 ;
        case KREDIT:
            return vienti.value("kreditsnt").toLongLong() / 100.0;
        }
    }
    else if( role == Qt::TextAlignmentRole)
    {
        if( index.column()==KREDIT || index.column() == DEBET || index.column() == ALV)
            return QVariant(Qt::AlignRight | Qt::AlignVCenter);
        else
            return QVariant( Qt::AlignLeft | Qt::AlignVCenter);

    }

    return QVariant();
}

bool TositeModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
    if( role == Qt::EditRole)
    {
        switch ( index.column()) {
        case PVM:
            viennit_[ index.row() ].insert("pvm", value.toDate() );
            break;
        case DEBET:
            viennit_[index.row()].insert("debet", value.toDouble() );
            viennit_[index.row()].remove("kredit");
            break;
        case KREDIT:
            viennit_[index.row()].insert("kredit", value.toDouble() );
            viennit_[index.row()].remove("debet");
            break;
        case SELITE:
            viennit_[index.row() ].insert("selite", value);
        }
        return true;

    }
    return false;
}

Qt::ItemFlags TositeModel::flags(const QModelIndex &index) const
{
    return QAbstractTableModel::flags(index) | Qt::ItemIsEditable;
}

Tositelaji TositeModel::tositelaji() const
{
    return kp()->tositelajit()->tositelajiVanha(  map_.value("tositelaji").toInt() );
}

bool TositeModel::muokkausSallittu() const
{
    // Jos päätetyllä tilikaudella, niin
    // ei saa muokata
    return  !pvm().isValid() ||  ( pvm() >= kp()->tilitpaatetty() &&
            kp()->tilitpaatetty() < kp()->tilikaudet()->kirjanpitoLoppuu() );
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
    if( tositelajiId < 0)
        tunniste_ = 0;

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

    KpKysely* kpkysely = kpk( QString("/tositteet/%1").arg(id) );
    connect( kpkysely, &KpKysely::vastaus, this, &TositeModel::lataaMapista);
    kpkysely->kysy();

    return;


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
    QVariantMap map(map_);

    QVariantList viennit;
    for(auto vienti : viennit_)
        viennit.append(vienti);

    map.insert("viennit", viennit);

    map.insert("liitteet", liitteet_.tallennettavat());


    qDebug() << QJsonDocument::fromVariant( map ).toJson();

    return false;


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

    if( tositelaji_ > -1)
        kysely.bindValue(":laji", tositelaji_ );
    else
        kysely.bindValue(":laji", QVariant() );

    if( tiliotetili())
        kysely.bindValue(":tiliote", tiliotetili());
    else
        kysely.bindValue(":tiliote", QVariant());

    kysely.bindValue(":json", json_.toSqlJson());
    kysely.bindValue(":muokattu", QDateTime::currentDateTime());

    if( !kysely.exec() )
    {
        kp()->lokiin(kysely);
        tietokanta()->rollback();
        return false;
    }


    if( id() < 0)
        id_ = kysely.lastInsertId().toInt();

    if( !vientiModel_->tallenna() || !liiteModel_->tallenna() )
    {
        // Tallennuksessa virheitä, perutaan ja palautetaan virhe
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

    if( !id())
        return true;


    // Alv-ilmoituksen poistaminen kääntää alv-lukko taaksepäin
    if( json()->date("AlvTilitysAlkaa").isValid() && json()->date("AlvTilitysPaattyy") == kp()->asetukset()->pvm("AlvIlmoitus"))
        kp()->asetukset()->aseta("AlvIlmoitus", json()->date("AlvTilitysAlkaa").addDays(-1));

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

void TositeModel::lataaMapista(QVariantMap *data, int status)
{
    beginResetModel();
    map_ = data->value("tosite").toMap();
    viennit_.clear();


    QVariant vientiVar = map_.take("viennit");
    for( QVariant var : vientiVar.toList())
        viennit_.append( var.toMap() );

    QVariant liiteVar = map_.take("liitteet");
    liitteet_.lataa( id(), liiteVar.toList() );

    endResetModel();

    qDebug() << "ladattu..." << status;

    emit tyhjennetty();
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

RaportinKirjoittaja TositeModel::selvittelyTuloste()
{
    RaportinKirjoittaja rk;
    rk.asetaOtsikko("TOSITTEEN TIEDOT");
    rk.asetaKausiteksti(QString("%1%2/%3")
                        .arg(tositelaji().tunnus()).arg(tunniste()).arg( kp()->tilikaudet()->tilikausiPaivalle( pvm() ).kausitunnus()));

    rk.lisaaSarake("XXXXXXXXXXXXXXX");
    rk.lisaaVenyvaSarake();

    QSqlQuery kysely( QString("SELECT * FROM tosite WHERE id=%1").arg(id()) );
    while(kysely.next())
    {
        QSqlRecord tietue = kysely.record();
        for(int i=0; i < tietue.count(); i++)
        {
            RaporttiRivi rr;
            QString avain = tietue.fieldName(i);
            QString arvo = tietue.value(i).toString();

            if( avain == "laji")
            {
                Tositelaji laji = kp()->tositelajit()->tositelajiVanha( arvo.toInt() );
                arvo.append(QString(" [%1 %2]").arg(laji.tunnus()).arg(laji.nimi()));
            }

            rr.lisaa( avain );
            rr.lisaa( arvo );
            rk.lisaaRivi( rr );
        }
    }


    kysely.exec( QString("SELECT * FROM vienti WHERE tosite=%1").arg(id()));
    while(kysely.next())
    {
        rk.lisaaRivi();
        QSqlRecord tietue = kysely.record();
        for(int i=0; i < tietue.count(); i++)
        {
            RaporttiRivi rr;
            QString avain = tietue.fieldName(i);
            QString arvo = tietue.value(i).toString();

            if( avain == "tili")
            {
                Tili tili = kp()->tilit()->tiliIdllaVanha( arvo.toInt() );
                if( tili.onkoValidi())
                    arvo.append(QString(" [%1 %2 %3]").arg(tili.numero()).arg(tili.nimi()).arg( tili.tyyppiKoodi() ) );
            }
            else if( avain == "kohdennus" && arvo.toInt())
            {
                Kohdennus kohdennus = kp()->kohdennukset()->kohdennus( arvo.toInt() );
                arvo.append( QString(" [%1]").arg(kohdennus.nimi()));
            }

            rr.lisaa( avain );
            rr.lisaa( arvo );
            rk.lisaaRivi( rr );
        }
    }

    // Liitteet - id ja otsikko
    if( liiteModel()->rowCount(QModelIndex()))
    {
        rk.lisaaRivi();
        RaporttiRivi rr;
        rr.lisaa("Liitteet",2);
        rr.lihavoi();
        rk.lisaaRivi(rr);

        for(int liite=0; liite < liiteModel()->rowCount(QModelIndex()); liite++)
        {
            RaporttiRivi lr;
            QModelIndex lInd = liiteModel()->index(liite,0);
            lr.lisaa( QString::number( lInd.data(LiiteModel::IdRooli).toInt() ));
            lr.lisaa(lInd.data(LiiteModel::OtsikkoRooli ).toString());
            rk.lisaaRivi(lr);
        }
    }
    rk.lisaaRivi();
    RaporttiRivi rr;
    rr.lisaa("Kitupiikin versio");
    rr.lisaa( QString("%1 %2 %3 %4")
              .arg( KITUPIIKKI_VERSIO )
              .arg( KITUPIIKKI_BUILD)
              .arg( AloitusSivu::buildDate().toString("dd.MM.yyyy"))
              .arg( QSysInfo::prettyProductName() ));
    rk.lisaaRivi(rr);
    return rk;
}

