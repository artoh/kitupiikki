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
#ifndef KIERTOMUOKKAUSMODEL_H
#define KIERTOMUOKKAUSMODEL_H

#include <QAbstractTableModel>

class KiertoMuokkausModel : public QAbstractTableModel
{
    Q_OBJECT

public:

    enum {
        NIMI,
        ROOLI,
        ILMOITA
    };

    enum {
        RooliRooli = Qt::UserRole
    };

    explicit KiertoMuokkausModel(QObject *parent = nullptr);

    // Header:
    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;


    // Basic functionality:
    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    int columnCount(const QModelIndex &parent = QModelIndex()) const override;

    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;

    // Editable:
    bool setData(const QModelIndex &index, const QVariant &value,
                 int role = Qt::EditRole) override;

    Qt::ItemFlags flags(const QModelIndex& index) const override;

    void lisaaRivi(const QVariantMap& rivi);
    void poistaRivi(int indeksi);
    void lataa(const QVariantList& lista);
    void lisaaNimet(const QMap<int,QString> nimet);
    QVariantList kiertoLista() const;

private:
    QVariantList lista_;
    QMap<int,QString> nimicache_;
};

#endif // KIERTOMUOKKAUSMODEL_H
