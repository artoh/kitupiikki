/*
   Copyright (C) 2026 Kitsas contributors

   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.
*/
#ifndef POSTGRESMODEL_H
#define POSTGRESMODEL_H

#include "sql/sqlmodel.h"
#include "postgresyhteys.h"

class PostgresModel : public SqlModel
{
    Q_OBJECT

public:
    enum { AvainRooli = Qt::UserRole, NimiRooli = Qt::UserRole + 2 };

    explicit PostgresModel(QObject *parent = nullptr);

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;

    bool avaa(const PostgresYhteys& yhteys, bool ilmoitaVirheesta = true);
    bool uusiKirjanpito(const PostgresYhteys& yhteys, const QVariantMap& initials);

    QStringList listaaTietokannat(const PostgresYhteys& palvelin, bool ilmoitaVirheesta = true);
    bool luoTietokanta(const PostgresYhteys& palvelin, const QString& nimi, bool ilmoitaVirheesta = true);
    static bool onkoKelvollinenTietokannanNimi(const QString& nimi);

    void lataaViimeiset();
    void poistaListalta(const PostgresYhteys& yhteys);

    PostgresYhteys nykyinenYhteys() const { return nykyinen_; }
    PostgresYhteys yhteysRivilla(int rivi) const;

    void sulje() override;

private slots:
    void lisaaViimeisiin();

private:
    bool yhdista(const PostgresYhteys& yhteys, bool ilmoitaVirheesta);
    bool onkoKaavioOlemassa();
    QSqlDatabase avaaHallinta(const PostgresYhteys& palvelin, bool ilmoitaVirheesta);

    PostgresYhteys nykyinen_;
    QVariantList viimeiset_;
};

#endif // POSTGRESMODEL_H
