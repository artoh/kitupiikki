/*
   Copyright (C) 2019 Arto Hyvättinen
   Copyright (C) 2026 Kitsas contributors

   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.
*/
#include "sqlmodel.h"

#include "db/kirjanpito.h"
#include "sqlite/sqlitekysely.h"
#include "sqlite/sqliteroute.h"

#include "sqlite/routes/initroute.h"
#include "sqlite/routes/tositeroute.h"
#include "sqlite/routes/viennitroute.h"
#include "sqlite/routes/kumppanitroute.h"
#include "sqlite/routes/liitteetroute.h"
#include "sqlite/routes/asetuksetroute.h"
#include "sqlite/routes/tilikaudetroute.h"
#include "sqlite/routes/saldotroute.h"
#include "sqlite/routes/asiakkaatroute.h"
#include "sqlite/routes/budjettiroute.h"
#include "sqlite/routes/eraroute.h"
#include "sqlite/routes/myyntilaskutroute.h"
#include "sqlite/routes/ostolaskutroute.h"
#include "sqlite/routes/toimittajatroute.h"
#include "sqlite/routes/kohdennusroute.h"
#include "sqlite/routes/tuotteetroute.h"
#include "sqlite/routes/tilitroute.h"
#include "sqlite/routes/alvroute.h"
#include "sqlite/routes/ryhmatroute.h"
#include "sqlite/routes/tuontitulkki.h"
#include "sqlite/routes/inforoute.h"
#include "sqlite/routes/vakioviiteroute.h"

#include <QSqlQuery>

SqlModel::SqlModel(const QString &ajuri, const QString &yhteysnimi, QObject *parent)
    : YhteysModel(parent)
{
    tietokanta_ = QSqlDatabase::addDatabase(ajuri, yhteysnimi);

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

SqlModel::~SqlModel()
{
    for( auto route : routes_)
        delete route;
}

KpKysely *SqlModel::kysely(const QString &polku, KpKysely::Metodi metodi)
{
    return new SQLiteKysely(this, metodi, polku);
}

void SqlModel::sulje()
{
    if( tietokanta_.isOpen()) {
        tietokanta_.exec("DELETE FROM Liite WHERE tosite IS NULL");
        tietokanta_.close();
    }
}

qlonglong SqlModel::oikeudet() const
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

void SqlModel::reitita(SQLiteKysely* reititettavakysely, const QVariant &data)
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

void SqlModel::reitita(SQLiteKysely *reititettavakysely, const QByteArray &ba, const QMap<QString, QString> &meta)
{
    for( SQLiteRoute* route : routes_) {
        if( reititettavakysely->polku().startsWith( route->polku() ) ) {
            reititettavakysely->vastaaLisayksesta( route->byteArray(reititettavakysely, ba, meta) );
            return;
        }
    }
    emit reititettavakysely->virhe(404);
}

qint64 SqlModel::tietokannanKoko() const
{
    return 0;
}

void SqlModel::lisaaRoute(SQLiteRoute *route)
{
    routes_.append(route);
}

void SqlModel::varmistaUid()
{
    QSqlQuery query( tietokanta_ );
    query.exec("SELECT Arvo FROM Asetus WHERE Avain='UID'");
    if(!query.next())
        query.exec(QString("INSERT INTO Asetus(Avain,Arvo) VALUES('UID','%1')").arg(Kirjanpito::satujono(16)));
}

void SqlModel::merkitseAvatuksi()
{
    tietokanta_.exec("UPDATE Asetus SET arvo=CURRENT_TIMESTAMP WHERE avain='Avattu'");
}
