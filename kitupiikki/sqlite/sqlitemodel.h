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

#include <QAbstractListModel>

class SQLiteModel : public QAbstractListModel
{
    Q_OBJECT

public:
    enum { PolkuRooli = Qt::UserRole, NimiRooli = Qt::UserRole + 2};

    SQLiteModel(QObject *parent = nullptr);

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;

    bool avaaTiedosto(const QString& polku, bool ilmoitavirheestaAvattaessa = true);
    void lataaViimeiset();

    void poistaListalta(const QString& polku);

private slots:
    void lisaaViimeisiin(bool onnistuiko);

private:
    QVariantList viimeiset_;
};

#endif // SQLITEMODEL_H
