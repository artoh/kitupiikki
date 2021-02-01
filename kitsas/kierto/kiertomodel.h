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
#ifndef KIERTOMODEL_H
#define KIERTOMODEL_H

#include <QAbstractListModel>

class KiertoModel : public QAbstractListModel
{
    Q_OBJECT

public:
    explicit KiertoModel(QObject *parent = nullptr);

    enum {
        NimiRooli = Qt::DisplayRole,
        IdRooli = Qt::UserRole
    };

    enum { ID, NIMI, TYYPPI};

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;

    QString nimi(int id) const;
    void lataaData(QVariant* lista);
    void lataa(const QVariantList &lista);
    void paivita();

private:
    QVariantList lista_;
};

#endif // KIERTOMODEL_H
