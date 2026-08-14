/*
   Copyright (C) 2019 Arto Hyvättinen
   Copyright (C) 2026 Kitsas contributors

   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.
*/
#ifndef SQLMODEL_H
#define SQLMODEL_H

#include "db/yhteysmodel.h"

#include <QSqlDatabase>

class SQLiteRoute;
class SQLiteKysely;

/**
 * @brief Yhteinen kanta paikallisille SQL-taustajärjestelmille (SQLite, PostgreSQL)
 *
 * Omistaa QSqlDatabase-yhteyden ja reitittää KpKyselyt samoille route-luokille.
 * SQLite-kohtaiset PRAGMA- ja tiedostotoiminnot jäävät SQLiteModeliin.
 */
class SqlModel : public YhteysModel
{
    Q_OBJECT

public:
    /**
     * @brief Käytössä oleva tietokantaversio
     *
     * Jos yritetään avata uudempaa, tulee virhe
     */
    static const int TIETOKANTAVERSIO = 24;

    ~SqlModel() override;

    KpKysely* kysely(const QString& polku = QString(),
                     KpKysely::Metodi metodi = KpKysely::GET) override;

    void sulje() override;

    QSqlDatabase tietokanta() const { return tietokanta_; }

    qlonglong oikeudet() const override;

    void reitita(SQLiteKysely *reititettavakysely, const QVariant& data);
    void reitita(SQLiteKysely* reititettavakysely, const QByteArray &ba, const QMap<QString,QString> &meta);

    virtual qint64 tietokannanKoko() const;

protected:
    SqlModel(const QString& ajuri, const QString& yhteysnimi, QObject *parent = nullptr);

    void lisaaRoute(SQLiteRoute *route);
    void varmistaUid();
    void merkitseAvatuksi();

    QSqlDatabase tietokanta_;

private:
    QList<SQLiteRoute*> routes_;
};

#endif // SQLMODEL_H
