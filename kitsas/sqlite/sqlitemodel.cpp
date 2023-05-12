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
#include "sqlitemodel.h"
#include "db/kirjanpito.h"
#include "sqlitekysely.h"

#include "sqlitealustaja.h"

#include <QSettings>
#include <QImage>
#include <QMessageBox>
#include <QDebug>
#include <QSqlError>
#include <QSqlQuery>
#include <QApplication>
#include <QJsonDocument>
#include <QRegularExpression>

#include "routes/initroute.h"
#include "routes/tositeroute.h"
#include "routes/viennitroute.h"
#include "routes/kumppanitroute.h"
#include "routes/liitteetroute.h"
#include "routes/asetuksetroute.h"
#include "routes/tilikaudetroute.h"
#include "routes/saldotroute.h"
#include "routes/asiakkaatroute.h"
#include "routes/budjettiroute.h"
#include "routes/eraroute.h"
#include "routes/myyntilaskutroute.h"
#include "routes/ostolaskutroute.h"
#include "routes/toimittajatroute.h"
#include "routes/kohdennusroute.h"
#include "routes/tuotteetroute.h"
#include "routes/tilitroute.h"
#include "routes/alvroute.h"
#include "routes/ryhmatroute.h"
#include "routes/tuontitulkki.h"
#include "routes/inforoute.h"
#include "routes/vakioviiteroute.h"

#include "versio.h"

#include <QPalette>

SQLiteModel::SQLiteModel(QObject *parent)
    : YhteysModel(parent)
{
    tietokanta_ = QSqlDatabase::addDatabase("QSQLITE", "KIRJANPITO");

    lisaaRoute(new TositeRoute(this));
    lisaaRoute(new ViennitRoute(this));
    lisaaRoute(new KumppanitRoute(this));
    lisaaRoute(new LiitteetRoute(this));
    lisaaRoute(new InitRoute(this));
    lisaaRoute(new SaldotRoute(this));
    lisaaRoute(new TilikaudetRoute(this));
    lisaaRoute(new AsetuksetRoute(this));
    lisaaRoute(new AsiakkaatRoute(this));
    lisaaRoute(new BudjettiRoute(this));
    lisaaRoute(new EraRoute(this));
    lisaaRoute(new MyyntilaskutRoute(this));
    lisaaRoute(new OstolaskutRoute(this));
    lisaaRoute(new ToimittajatRoute(this));
    lisaaRoute(new KohdennusRoute(this));
    lisaaRoute(new TuotteetRoute(this));
    lisaaRoute(new TilitRoute(this));
    lisaaRoute(new RyhmatRoute(this));
    lisaaRoute(new TuontiTulkki(this));
    lisaaRoute(new AlvRoute(this));
    lisaaRoute(new VakioviiteRoute(this));
    lisaaRoute(new InfoRoute(this));
}

SQLiteModel::~SQLiteModel()
{
    for( auto route : routes_)
        delete route;
}


int SQLiteModel::rowCount(const QModelIndex &parent) const
{
    if (parent.isValid())
        return 0;
    return viimeiset_.count();
}

QVariant SQLiteModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid())
        return QVariant();

    QVariantMap map = viimeiset_.at(index.row()).toMap();
    switch (role) {
    case Qt::DisplayRole:
    case NimiRooli:
        return map.value("nimi").toString();
    case Qt::DecorationRole:
    {
        QImage image = map.value("logo").value<QImage>();
        if( image.isNull())
            return QPixmap(":/pic/tyhja16.png");
        else
            return QPixmap::fromImage(image);
    }
    case PolkuRooli:
        {
            QString polku = map.value("polku").toString();
            // Palautetaan aina täydellinen polku
            QDir portableDir( kp()->portableDir() );
            if( !kp()->portableDir().isEmpty())
                polku = QDir::cleanPath(portableDir.absoluteFilePath(polku));
            return polku;
        }
    }
    if( role == Qt::ForegroundRole) {
        if( map.value("harjoitus").toBool()) {
            if( QPalette().base().color().lightness() > 128)
                return QColor(Qt::darkGreen);
            else
                return QColor(Qt::green);
        }
        const QString& polku = map.value("polku").toString();
        if( polku.contains(QRegularExpression("\\d{6}")) ) {
            if( QPalette().base().color().lightness() > 128)
                return QColor(Qt::darkMagenta);
            else
                return QColor(Qt::magenta);
        }
    }

    return QVariant();
}

bool SQLiteModel::avaaTiedosto(const QString &polku, bool ilmoitavirheestaAvattaessa, bool asetaAktiiviseksi)
{

    kp()->odotusKursori(true);

    tietokanta_.setDatabaseName( polku );
    tiedostoPolku_.clear();
    if( asetaAktiiviseksi)
        kp()->yhteysAvattu(nullptr);

    if( !tietokanta_.open())
    {
        kp()->odotusKursori(false);
        if( ilmoitavirheestaAvattaessa ) {
            QMessageBox::critical(nullptr, tr("Tietokannan avaaminen epäonnistui"),
                                  tr("Tietokannan %1 avaaminen epäonnistui tietokantavirheen %2 takia")
                                  .arg( polku ).arg( tietokanta().lastError().text() ) );
        }
        qWarning() << "SQLiteYhteys: Tietokannan avaaminen epäonnistui : " << tietokanta_.lastError().text();
        return false;
    }

    // Lukitaan tietokanta, jotta käyttäminen on varmasti turvallista
#ifndef KITSAS_DEVEL
    tietokanta_.exec("PRAGMA LOCKING_MODE = EXCLUSIVE");
#endif
    tietokanta_.exec("PRAGMA JOURNAL_MODE = WAL");

    QSqlQuery query( tietokanta_ );
    query.exec("SELECT arvo FROM Asetus WHERE avain='KpVersio'");

    if( query.lastError().isValid() )
    {
        kp()->odotusKursori(false);
        // Tietokanta on jo käytössä
        if( ilmoitavirheestaAvattaessa )
        {
            if( query.lastError().text().contains("locked"))
            {
                QMessageBox::critical(nullptr, tr("Kirjanpitoa ei voi avata"),
                                      tr("Kirjanpitotiedosto %1 on jo käytössä.\n\n"
                                         "Sulje kaikki Kitsas-ohjelman ikkunat ja yritä uudelleen.\n\n"
                                         "Ellei tämä auta, käynnistä tietokoneesi uudelleen.").arg(polku));
            }
            else if( query.lastError().text().contains("no such table: Asetus") ) {
                QMessageBox::critical(nullptr,
                                      tr("Kirjanpitoa ei voi avata"),
                                      tr("Kirjanpitotiedosto %1 on todennäköisesti vahingoittunut joko tiedostojärjestelmän virheen tai levyvirheen takia, tai tiedosto ei ole Kitsaan kirjanpitotiedosto.").arg(polku) + "\n\n" +
                                      tr("Ellei tietokoneen uudelleen käynnistäminen auta, ota käyttöön kirjanpitosi varmuuskopio."));
            }
            else
            {
                QMessageBox::critical(nullptr,
                                      tr("Kirjanpitoa ei voi avata"),
                                      tr("Tiedostoa %1 ei voi avata").arg(polku) + "\n" +
                                      tr("Sql-virhe: %1").arg(query.lastError().text()));
            }
        }
        tietokanta_.close();
        return false;
    }
    // Tarkastetaan versio
    if( query.next()) {
        int versio = query.value(0).toInt();
        if( versio > TIETOKANTAVERSIO) {
            kp()->odotusKursori(false);
            QMessageBox::critical(nullptr, tr("Kirjanpitoa %1 ei voi avata").arg(polku),
                                  tr("Kirjanpito on luotu uudemmalla Kitsaan versiolla, eikä käytössäsi oleva versio %1 pysty avaamaan sitä.\n\n"
                                     "Voidaksesi avata tiedoston, sinun on asennettava uudempi versio Kitsaasta. Lataa ohjelma "
                                     "osoitteesta https://kitsas.fi")
                                  .arg( qApp->applicationVersion() ));
            tietokanta_.close();
            return false;
        } else if( versio < TIETOKANTAVERSIO) {
            kp()->odotusKursori(false);
            if(QMessageBox::question(nullptr, tr("Kirjanpidon päivittäminen"),
                                     tr("Avataksesi kirjanpidon pitää se päivittää yhteensopivaksi nykyisen version kanssa. Päivityksen jälkeen kirjanpitoa ei voi enää avata varhaisemmilla esiversioilla. "
                                        "\nPäivitetäänkö kirjanpito nyt?"), QMessageBox::Yes | QMessageBox::Cancel, QMessageBox::Cancel)!= QMessageBox::Yes) {
                tietokanta_.close();
                return false;
            }
            kp()->odotusKursori(false);
            // #603 IBAN siirretään omaan tietokantakenttään, jotta säilyy päivitysten ylitse
            if( versio < 24) {
                query.exec("ALTER TABLE Tili ADD COLUMN iban VARCHAR(32)");
                QSqlQuery ibanquery( tietokanta_ );
                ibanquery.exec("SELECT numero,json FROM Tili WHERE tyyppi='ARP'");
                while(ibanquery.next()) {
                    QVariantMap map = QJsonDocument::fromJson(ibanquery.value("json").toByteArray()).toVariant().toMap();
                    if( map.contains("JSON")) {
                        query.exec(QString("UPDATE Tili SET IBAN='%1' WHERE numero=%2").arg(ibanquery.value("numero").toInt()).arg(map.value("IBAN").toString()));
                    }
                }
            }
            // #539 Vakioviitteiden taulun luominen
            if( versio < 23 )
                query.exec("CREATE TABLE Vakioviite ( viite integer PRIMARY KEY NOT NULL, tili INTEGER REFERENCES Tili(numero) ON DELETE CASCADE, kohdennus INTEGER REFERENCES Kohdennus(id) ON DELETE CASCADE, "
                        " otsikko TEXT, alkaen DATE, paattyen DATE, json TEXT) ");
            if( versio < 22) {
                // Ensimmäisen version jälkeen on lisätty kenttä laskupäivälle
                query.exec("ALTER TABLE Tosite ADD COLUMN laskupvm DATE");
                query.exec("ALTER TABLE Tosite ADD COLUMN erapvm DATE");
                query.exec("ALTER TABLE Tosite ADD COLUMN viite TEXT");
                query.exec("ALTER TABLE Vienti ADD COLUMN arkistotunnus TEXT");
                query.exec("UPDATE Tosite SET erapvm =(SELECT MAX(erapvm) FROM Vienti WHERE Vienti.tosite=Tosite.id");
                query.exec("UPDATE Tosite SET viite =(SELECT MAX(viite) FROM Vienti WHERE Vienti.tosite=Tosite.id");

                if( versio == 21) {
                    query.exec("UPDATE Tosite SET laskupvm =(SELECT MAX(laskupvm) FROM Vienti WHERE Vienti.tosite=Tosite.id");
                } else {
                    query.exec("UPDATE Tosite SET laskupvm=pvm");
                }
            }
            query.exec(QString("UPDATE Asetus SET arvo=%1 WHERE avain='KpVersio'").arg(TIETOKANTAVERSIO));
        }
    } else {
        // Tämä ei ole lainkaan kelvollinen tietokanta
        kp()->odotusKursori(false);
        QMessageBox::critical(nullptr, tr("Tiedostoa %1 ei voi avata").arg(polku),
                              tr("Valitsemasi tiedosto ei ole Kitsaan tietokanta, tai tiedosto on vahingoittunut."));
        qWarning() << tietokanta_.lastError().text();
        tietokanta_.close();
        return false;
    }        

    // Varmistetaan, että kaikilla kirjanpidoilla on UID, jota käytetään
    // esim. arkistohakemiston sijainnin tallettamiseen
    query.exec("SELECT Arvo FROM Asetus WHERE Avain='UID'");
    if(!query.next())
        query.exec(QString("INSERT INTO Asetus(Avain,Arvo) VALUES('UID','%1')").arg(Kirjanpito::satujono(16)));


    // Merkitään avausaika
    tietokanta_.exec("UPDATE Asetus SET arvo=CURRENT_TIMESTAMP WHERE avain='Avattu'");


    tiedostoPolku_ = polku;

    alusta();
    if( asetaAktiiviseksi)
        lisaaViimeisiin();   

    connect( kp(), &Kirjanpito::perusAsetusMuuttui, this, &SQLiteModel::lisaaViimeisiin );

    return true;
}

void SQLiteModel::lataaViimeiset()
{
    beginResetModel();
    viimeiset_ = kp()->settings()->value("ViimeTiedostot").toList();

    QDir portableDir( kp()->portableDir() );

    QMutableListIterator<QVariant> iter( viimeiset_ );
    while( iter.hasNext())
    {
        QString polku = iter.next().toMap().value("polku").toString();
        if( !kp()->portableDir().isEmpty())
            polku = QDir::cleanPath(portableDir.absoluteFilePath(polku));
        if( !QFile::exists(polku))
            iter.remove();
    }

    kp()->settings()->setValue("ViimeTiedostot", viimeiset_);
    endResetModel();

}

void SQLiteModel::poistaListalta(const QString &polku)
{

    QDir portableDir( kp()->portableDir() );
    QString poistettava = kp()->portableDir().isEmpty() ? polku : portableDir.relativeFilePath(polku);

    beginResetModel();
    QMutableListIterator<QVariant> iter( viimeiset_ );
    while( iter.hasNext())
    {
        QString tamanpolku = iter.next().toMap().value("polku").toString();
        if( poistettava == tamanpolku )
            iter.remove();
    }
    endResetModel();
    kp()->settings()->setValue("ViimeTiedostot", viimeiset_);
    emit kp()->yhteysAvattu(nullptr);

}

KpKysely *SQLiteModel::kysely(const QString &polku, KpKysely::Metodi metodi)
{
    return new SQLiteKysely(this, metodi, polku);
}

void SQLiteModel::sulje()
{
    tietokanta_.exec("DELETE FROM Liite WHERE tosite IS NULL");
    tietokanta_.close();
    tiedostoPolku_.clear();
    disconnect( kp(), &Kirjanpito::perusAsetusMuuttui, this, &SQLiteModel::lisaaViimeisiin );
}

qlonglong SQLiteModel::oikeudet() const
{
    return TOSITE_SELAUS |
            TOSITE_LUONNOS |
            TOSITE_MUOKKAUS |
            LASKU_SELAUS |
            LASKU_LAATIMINEN |
            LASKU_LAHETTAMINEN |
            ALV_ILMOITUS |
            BUDJETTI |
            TILINPAATOS |
            ASETUKSET |
            TUOTTEET |
            RYHMAT |
            RAPORTIT;
}

bool SQLiteModel::uusiKirjanpito(const QString &polku, const QVariantMap &initials)
{
    return SqliteAlustaja::luoKirjanpito(polku, initials);
}

void SQLiteModel::reitita(SQLiteKysely* reititettavakysely, const QVariant &data)
{
    qInfo() << reititettavakysely->polku() + " " + reititettavakysely->urlKysely().toString();

    for( SQLiteRoute* route : routes_) {
        if( reititettavakysely->polku().startsWith( route->polku() ) ) {
            reititettavakysely->vastaa(route->route( reititettavakysely, data));
            return;
        }
    }
    qWarning() << " *** Kyselyä " << reititettavakysely->polku() << " ei reititetty ***";
    emit reititettavakysely->virhe(404);
}

void SQLiteModel::reitita(SQLiteKysely *reititettavakysely, const QByteArray &ba, const QMap<QString, QString> &meta)
{
    for( SQLiteRoute* route : routes_) {
        if( reititettavakysely->polku().startsWith( route->polku() ) ) {
            reititettavakysely->vastaaLisayksesta( route->byteArray(reititettavakysely, ba, meta) );
            return;
        }
    }
    emit reititettavakysely->virhe(404);
}


void SQLiteModel::lisaaViimeisiin()
{

    QVariantMap map;
    // PORTABLE polut tallennetaan suhteessa portable-hakemistoon
    QDir portableDir( kp()->portableDir() );
    QString polku = tiedostopolku();

    if(polku.isEmpty())
        return;

    if( !kp()->portableDir().isEmpty())
        polku = portableDir.relativeFilePath(polku);

    map.insert("polku", tiedostopolku() );
    map.insert("nimi", kp()->asetukset()->asetus(AsetusModel::OrganisaatioNimi) );
    if( !kp()->logo().isNull())
        map.insert("logo", kp()->logo().scaled(16,16,Qt::KeepAspectRatio));
    map.insert("harjoitus", kp()->onkoHarjoitus());

    beginResetModel();
    for( int i=0; i < viimeiset_.count(); i++ )
    {
        if( viimeiset_.value(i).toMap().value("polku").toString() == map.value("polku").toString()) {
            viimeiset_[i] = map;
            endResetModel();
            kp()->settings()->setValue("ViimeTiedostot", viimeiset_);
            return;
        }
    }

    viimeiset_.append(map);
    endResetModel();

    kp()->settings()->setValue("ViimeTiedostot", viimeiset_);
}

void SQLiteModel::lisaaRoute(SQLiteRoute *route)
{
    routes_.append(route);
}
