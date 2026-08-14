/*
   Copyright (C) 2019 Arto Hyvättinen
   Copyright (C) 2026 Kitsas contributors

   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.
*/
#ifndef SQLALUSTAJA_H
#define SQLALUSTAJA_H

#include <QSqlDatabase>
#include <QVariantMap>

class QProgressDialog;

/**
 * @brief Yhteiset SQL-taustajärjestelmän alustustoiminnot
 */
class SqlAlustaja
{
public:
    static bool suoritaSqlResurssi(QSqlDatabase db, const QString& resurssi);
    static bool kirjoitaInit(QSqlDatabase db, const QVariantMap& initMap, QProgressDialog *progress = nullptr);

private:
    static QString json(const QVariant& var);
    static void aseta(QSqlDatabase db, const QString& avain, const QVariant& arvo);
    static void kirjoitaAsetukset(QSqlDatabase db, const QVariantMap& asetukset);
    static void kirjoitaTilit(QSqlDatabase db, const QVariantList& tililista);
    static void kirjoitaTilikaudet(QSqlDatabase db, const QVariantList& kausilista);
    static void kirjoitaAvausTosite(QSqlDatabase db, const QDate& tilinavauspaiva);
};

#endif // SQLALUSTAJA_H
