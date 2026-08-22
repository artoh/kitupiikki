/*
   Copyright (C) 2026 Kitsas contributors

   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.
*/
#ifndef TESTDB_H
#define TESTDB_H

#include "db/kpkysely.h"
#include "postgres/postgresyhteys.h"
#include "sql/sqlmodel.h"

#include <QSqlDatabase>
#include <QString>
#include <QStringList>
#include <QTemporaryDir>
#include <QVariant>
#include <QVariantList>
#include <QVariantMap>

/**
 * Opens a real Kitsas bookkeeping on SQLite and PostgreSQL through
 * Kirjanpito / SQLiteModel / PostgresModel (one backend at a time).
 */
class TestDb
{
public:
    static const QStringList kTables;

    bool alusta();
    void sulje();
    QString portablePolku() const { return portable_.path(); }

    bool avaaSqlite();
    bool avaaPostgres();

    bool postgresKaytossa() const { return postgresKaytossa_; }
    QString postgresVirhe() const { return postgresVirhe_; }

    SqlModel *sqlModel() const;
    QSqlDatabase sql() const;

    QVariant kysy(const QString& polku,
                  KpKysely::Metodi metodi = KpKysely::GET,
                  const QVariant& data = QVariant());
    QVariant lahetaTiedosto(const QString& polku,
                            const QByteArray& data,
                            const QMap<QString, QString>& meta = {});

    static QVariantMap initials();
    static PostgresYhteys postgresYhteys();

    static QString sqliteLuoSqlPolku();
    static QString postgresLuoSqlPolku();
    static QString lueTiedosto(const QString& polku);

    static QVariant normalisoi(const QVariant& arvo);
    static QVariantList dump(QSqlDatabase db, const QString& sql);
    static QStringList taulut(QSqlDatabase db);
    static QStringList sarakkeet(QSqlDatabase db, const QString& taulu);

    static QStringList luoSqlTaulut(const QString& sql);
    static QVariantMap luoSqlSarakkeet(const QString& sql);
    static QString normalisoiKaavioSql(const QString& sql);

private:
    bool pudotaJaLuoPostgresTietokanta();

    QTemporaryDir portable_;
    QString sqlitePolku_;
    bool postgresKaytossa_ = false;
    QString postgresVirhe_;
    QString postgresDbNimi_ = QStringLiteral("kitsas_parity");
};

#endif // TESTDB_H
