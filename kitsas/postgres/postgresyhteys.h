/*
   Copyright (C) 2026 Kitsas contributors

   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.
*/
#ifndef POSTGRESYHTEYS_H
#define POSTGRESYHTEYS_H

#include <QString>
#include <QVariantMap>

struct PostgresYhteys
{
    QString host = QStringLiteral("localhost");
    int port = 5432;
    QString database;
    QString username;
    QString password;

    QString avain() const {
        return QStringLiteral("%1:%2/%3").arg(host).arg(port).arg(database);
    }

    PostgresYhteys hallintaYhteys() const {
        PostgresYhteys tulos = *this;
        tulos.database = QStringLiteral("postgres");
        return tulos;
    }

    PostgresYhteys asiakasYhteys(const QString& tietokanta) const {
        PostgresYhteys tulos = *this;
        tulos.database = tietokanta;
        return tulos;
    }

    QVariantMap toMap(bool sisallytaSalasana = false) const {
        QVariantMap map;
        map.insert(QStringLiteral("host"), host);
        map.insert(QStringLiteral("port"), port);
        map.insert(QStringLiteral("database"), database);
        map.insert(QStringLiteral("username"), username);
        if (sisallytaSalasana)
            map.insert(QStringLiteral("password"), password);
        return map;
    }

    static PostgresYhteys fromMap(const QVariantMap& map) {
        PostgresYhteys yhteys;
        yhteys.host = map.value(QStringLiteral("host"), QStringLiteral("localhost")).toString();
        yhteys.port = map.value(QStringLiteral("port"), 5432).toInt();
        yhteys.database = map.value(QStringLiteral("database")).toString();
        yhteys.username = map.value(QStringLiteral("username")).toString();
        yhteys.password = map.value(QStringLiteral("password")).toString();
        return yhteys;
    }
};

#endif // POSTGRESYHTEYS_H
