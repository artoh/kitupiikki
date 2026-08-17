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
#ifndef SQLITEMODEL_H
#define SQLITEMODEL_H

#include "sql/sqlmodel.h"

class SQLiteModel : public SqlModel
{
    Q_OBJECT

public:
    enum { PolkuRooli = Qt::UserRole, NimiRooli = Qt::UserRole + 2};

    SQLiteModel(QObject *parent = nullptr);
    ~SQLiteModel() override = default;

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;

    bool avaaTiedosto(const QString& polku, bool ilmoitavirheestaAvattaessa = true, bool asetaAktiiviseksi = true);
    void lataaViimeiset();

    void poistaListalta(const QString& polku);

    void sulje() override;

    QString tiedostopolku() const { return tiedostoPolku_; }

    bool uusiKirjanpito(const QString& polku, const QVariantMap& initials);

    qint64 tietokannanKoko() const override;

private slots:
    void lisaaViimeisiin();

private:
    QVariantList viimeiset_;
    QString tiedostoPolku_;
};

#endif // SQLITEMODEL_H
