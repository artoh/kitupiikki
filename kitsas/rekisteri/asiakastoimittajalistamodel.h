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
#ifndef ASIAKASTOIMITTAJALISTAMODEL_H
#define ASIAKASTOIMITTAJALISTAMODEL_H

#include <QAbstractListModel>

class AsiakasToimittajaListaModel : public QAbstractListModel
{
    Q_OBJECT

private:
    struct Item {
        int id;
        QString nimi;
        QString alvtunnus;

        Item(int uId, QString uNimi, QString uAlvtunnus);
    };

public:
    enum {
        IdRooli = Qt::UserRole
    };


    explicit AsiakasToimittajaListaModel(QObject *parent = nullptr);

    // Basic functionality:
    int rowCount(const QModelIndex &parent = QModelIndex()) const override;

    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;

    int idAlvTunnuksella(const QString tunnus) const;
    int idNimella(const QString& nimi) const;
    QString nimi(int id) const;

    static AsiakasToimittajaListaModel *instanssi();

public slots:
    void lataa();

private slots:
    void saapuu(QVariant* variant);

private:
    QList<Item> lista_;

    static AsiakasToimittajaListaModel* instanssi__;
};

#endif // ASIAKASTOIMITTAJALISTAMODEL_H
