/*
   Copyright (C) 2026 Kitsas contributors

   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.
*/
#include "postgresmodel.h"

#include "db/kirjanpito.h"
#include "sql/sqlalustaja.h"

#include <QApplication>
#include <QMessageBox>
#include <QRegularExpression>
#include <QSettings>
#include <QSqlError>
#include <QSqlQuery>

PostgresModel::PostgresModel(QObject *parent)
    : SqlModel(QStringLiteral("QPSQL"), QStringLiteral("KIRJANPITO_PG"), parent)
{
}

int PostgresModel::rowCount(const QModelIndex &parent) const
{
    if (parent.isValid())
        return 0;
    return viimeiset_.count();
}

QVariant PostgresModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid())
        return QVariant();

    QVariantMap map = viimeiset_.at(index.row()).toMap();
    switch (role) {
    case Qt::DisplayRole:
    case NimiRooli:
        if( map.contains("nimi"))
            return map.value("nimi").toString();
        return QStringLiteral("%1@%2/%3")
                .arg(map.value("username").toString(),
                     map.value("host").toString(),
                     map.value("database").toString());
    case AvainRooli:
        return map.value("avain");
    }
    return QVariant();
}

PostgresYhteys PostgresModel::yhteysRivilla(int rivi) const
{
    if( rivi < 0 || rivi >= viimeiset_.count() )
        return PostgresYhteys();
    return PostgresYhteys::fromMap(viimeiset_.at(rivi).toMap());
}

bool PostgresModel::yhdista(const PostgresYhteys &yhteys, bool ilmoitaVirheesta)
{
    if( !QSqlDatabase::isDriverAvailable(QStringLiteral("QPSQL")) ) {
        if( ilmoitaVirheesta )
            QMessageBox::critical(nullptr, tr("PostgreSQL-yhteys"),
                                  tr("Qt:n QPSQL-ajuri ei ole käytettävissä."));
        return false;
    }

    if( tietokanta_.isOpen())
        tietokanta_.close();

    tietokanta_.setHostName(yhteys.host);
    tietokanta_.setPort(yhteys.port);
    tietokanta_.setDatabaseName(yhteys.database);
    tietokanta_.setUserName(yhteys.username);
    tietokanta_.setPassword(yhteys.password);

    if( !tietokanta_.open()) {
        if( ilmoitaVirheesta )
            QMessageBox::critical(nullptr, tr("PostgreSQL-yhteys epäonnistui"),
                                  tr("Yhteyttä tietokantaan %1@%2:%3/%4 ei voitu avata.\n%5")
                                  .arg(yhteys.username, yhteys.host)
                                  .arg(yhteys.port)
                                  .arg(yhteys.database, tietokanta_.lastError().text()));
        qWarning() << "PostgresModel: yhteys epäonnistui :" << tietokanta_.lastError().text();
        return false;
    }
    return true;
}

qint64 PostgresModel::tietokannanKoko() const
{
    if( !tietokanta_.isOpen())
        return 0;
    QSqlQuery kysely(tietokanta_);
    if( kysely.exec(QStringLiteral("SELECT pg_database_size(current_database())")) && kysely.next())
        return kysely.value(0).toLongLong();
    return 0;
}

bool PostgresModel::onkoKelvollinenTietokannanNimi(const QString &nimi)
{
    static const QRegularExpression kelpo(QStringLiteral("^[a-z][a-z0-9_]*$"));
    return kelpo.match(nimi).hasMatch()
            && nimi.size() <= 63
            && nimi != QLatin1String("postgres")
            && nimi != QLatin1String("template0")
            && nimi != QLatin1String("template1");
}

QSqlDatabase PostgresModel::avaaHallinta(const PostgresYhteys &palvelin, bool ilmoitaVirheesta)
{
    const QString yhteysnimi = QStringLiteral("KIRJANPITO_PG_ADMIN");
    if( QSqlDatabase::contains(yhteysnimi) ) {
        {
            QSqlDatabase vanha = QSqlDatabase::database(yhteysnimi);
            if( vanha.isOpen())
                vanha.close();
        }
        QSqlDatabase::removeDatabase(yhteysnimi);
    }

    QSqlDatabase hallinta = QSqlDatabase::addDatabase(QStringLiteral("QPSQL"), yhteysnimi);
    PostgresYhteys yhteys = palvelin;
    if( yhteys.database.isEmpty())
        yhteys.database = QStringLiteral("postgres");
    hallinta.setHostName(yhteys.host);
    hallinta.setPort(yhteys.port);
    hallinta.setDatabaseName(yhteys.database);
    hallinta.setUserName(yhteys.username);
    hallinta.setPassword(yhteys.password);

    if( !hallinta.open()) {
        if( ilmoitaVirheesta )
            QMessageBox::critical(nullptr, tr("PostgreSQL-yhteys epäonnistui"),
                                  tr("Yhteyttä palvelimeen %1:%2 ei voitu avata.\n%3")
                                  .arg(yhteys.host)
                                  .arg(yhteys.port)
                                  .arg(hallinta.lastError().text()));
        qWarning() << "PostgresModel: hallintayhteys epäonnistui :" << hallinta.lastError().text();
    }
    return hallinta;
}

QStringList PostgresModel::listaaTietokannat(const PostgresYhteys &palvelin, bool ilmoitaVirheesta)
{
    QStringList nimet;
    QSqlDatabase hallinta = avaaHallinta(palvelin, ilmoitaVirheesta);
    if( !hallinta.isOpen())
        return nimet;

    QSqlQuery query(hallinta);
    if( !query.exec(QStringLiteral(
            "SELECT datname FROM pg_database "
            "WHERE datistemplate = false AND datname <> 'postgres' "
            "ORDER BY datname")) ) {
        if( ilmoitaVirheesta )
            QMessageBox::critical(nullptr, tr("Tietokantojen haku epäonnistui"),
                                  query.lastError().text());
        hallinta.close();
        return nimet;
    }

    while( query.next())
        nimet.append(query.value(0).toString());

    hallinta.close();
    return nimet;
}

bool PostgresModel::luoTietokanta(const PostgresYhteys &palvelin, const QString &nimi, bool ilmoitaVirheesta)
{
    const QString tietokanta = nimi.trimmed().toLower();
    if( !onkoKelvollinenTietokannanNimi(tietokanta) ) {
        if( ilmoitaVirheesta )
            QMessageBox::warning(nullptr, tr("Uusi asiakas"),
                                 tr("Asiakkaan nimen on alettava kirjaimella ja se saa sisältää vain kirjaimia, numeroita ja alaviivoja (enintään 63 merkkiä)."));
        return false;
    }

    QSqlDatabase hallinta = avaaHallinta(palvelin, ilmoitaVirheesta);
    if( !hallinta.isOpen())
        return false;

    QSqlQuery query(hallinta);
    const bool ok = query.exec(QStringLiteral("CREATE DATABASE %1 ENCODING 'UTF8'").arg(tietokanta));
    if( !ok && ilmoitaVirheesta )
        QMessageBox::critical(nullptr, tr("Uusi asiakas"),
                              tr("Tietokannan %1 luominen epäonnistui.\n%2")
                              .arg(tietokanta, query.lastError().text()));

    hallinta.close();
    return ok;
}

bool PostgresModel::onkoKaavioOlemassa()
{
    QSqlQuery query(tietokanta_);
    query.exec("SELECT arvo FROM Asetus WHERE avain='KpVersio'");
    return query.lastError().type() == QSqlError::NoError;
}

bool PostgresModel::avaa(const PostgresYhteys &yhteys, bool ilmoitaVirheesta)
{
    kp()->odotusKursori(true);
    kp()->yhteysAvattu(nullptr);

    if( !yhdista(yhteys, ilmoitaVirheesta) ) {
        kp()->odotusKursori(false);
        return false;
    }

    if( !onkoKaavioOlemassa() ) {
        kp()->odotusKursori(false);
        if( ilmoitaVirheesta )
            QMessageBox::critical(nullptr, tr("Kirjanpitoa ei voi avata"),
                                  tr("Tietokannassa %1 ei ole Kitsaan kaaviota. Luo uusi kirjanpito ohjatulla toiminnolla.")
                                  .arg(yhteys.avain()));
        tietokanta_.close();
        return false;
    }

    QSqlQuery query(tietokanta_);
    query.exec("SELECT arvo FROM Asetus WHERE avain='KpVersio'");
    if( !query.next()) {
        kp()->odotusKursori(false);
        if( ilmoitaVirheesta )
            QMessageBox::critical(nullptr, tr("Kirjanpitoa ei voi avata"),
                                  tr("Tietokanta %1 ei ole Kitsaan kirjanpito.").arg(yhteys.avain()));
        tietokanta_.close();
        return false;
    }

    const int versio = query.value(0).toInt();
    if( versio > TIETOKANTAVERSIO) {
        kp()->odotusKursori(false);
        if( ilmoitaVirheesta )
            QMessageBox::critical(nullptr, tr("Kirjanpitoa ei voi avata"),
                                  tr("Kirjanpito on luotu uudemmalla Kitsaan versiolla, eikä käytössäsi oleva versio %1 pysty avaamaan sitä.")
                                  .arg(qApp->applicationVersion()));
        tietokanta_.close();
        return false;
    }
    if( versio < TIETOKANTAVERSIO) {
        kp()->odotusKursori(false);
        if( ilmoitaVirheesta )
            QMessageBox::critical(nullptr, tr("Kirjanpitoa ei voi avata"),
                                  tr("Tietokanta on vanhempaa versiota (%1). PostgreSQL-kaavion automaattista päivitystä ei ole vielä tässä versiossa.")
                                  .arg(versio));
        tietokanta_.close();
        return false;
    }

    varmistaUid();
    merkitseAvatuksi();
    nykyinen_ = yhteys;

    alusta();
    lisaaViimeisiin();
    connect( kp(), &Kirjanpito::perusAsetusMuuttui, this, &PostgresModel::lisaaViimeisiin );
    return true;
}

bool PostgresModel::uusiKirjanpito(const PostgresYhteys &yhteys, const QVariantMap &initials, bool ilmoitaVirheesta)
{
    if( !yhdista(yhteys, ilmoitaVirheesta) )
        return false;

    if( onkoKaavioOlemassa() ) {
        if( ilmoitaVirheesta )
            QMessageBox::critical(nullptr, tr("Tietokanta ei ole tyhjä"),
                                  tr("Tietokannassa %1 on jo Kitsaan kaavio. Valitse tyhjä tietokanta tai avaa olemassa oleva kirjanpito.")
                                  .arg(yhteys.avain()));
        tietokanta_.close();
        return false;
    }

    if( !SqlAlustaja::suoritaSqlResurssi(tietokanta_, QStringLiteral(":/postgres/luo.sql")) ) {
        tietokanta_.close();
        return false;
    }

    QVariantMap initMap = initials.value("init").toMap();
    if( !SqlAlustaja::kirjoitaInit(tietokanta_, initMap) ) {
        tietokanta_.close();
        return false;
    }

    tietokanta_.close();
    return avaa(yhteys, ilmoitaVirheesta);
}

void PostgresModel::lataaViimeiset()
{
    beginResetModel();
    viimeiset_ = kp()->settings()->value("ViimePostgres").toList();
    endResetModel();
}

void PostgresModel::poistaListalta(const PostgresYhteys &yhteys)
{
    beginResetModel();
    QMutableListIterator<QVariant> iter(viimeiset_);
    while( iter.hasNext()) {
        if( iter.next().toMap().value("avain").toString() == yhteys.avain() )
            iter.remove();
    }
    endResetModel();
    kp()->settings()->setValue("ViimePostgres", viimeiset_);
    emit kp()->yhteysAvattu(nullptr);
}

void PostgresModel::sulje()
{
    nykyinen_ = PostgresYhteys();
    disconnect( kp(), &Kirjanpito::perusAsetusMuuttui, this, &PostgresModel::lisaaViimeisiin );
    SqlModel::sulje();
}

void PostgresModel::lisaaViimeisiin()
{
    if( nykyinen_.database.isEmpty())
        return;

    // Password intentionally excluded: this list is persisted to QSettings
    // in plaintext, unlike the initial-connect path in aloitussivu.cpp which
    // already deliberately omits it.
    QVariantMap map = nykyinen_.toMap();
    map.insert("avain", nykyinen_.avain());
    map.insert("nimi", kp()->asetukset()->asetus(AsetusModel::OrganisaatioNimi) );

    beginResetModel();
    for( int i=0; i < viimeiset_.count(); i++ ) {
        if( viimeiset_.value(i).toMap().value("avain").toString() == map.value("avain").toString()) {
            viimeiset_[i] = map;
            endResetModel();
            kp()->settings()->setValue("ViimePostgres", viimeiset_);
            return;
        }
    }
    viimeiset_.append(map);
    endResetModel();
    kp()->settings()->setValue("ViimePostgres", viimeiset_);
}
