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
#ifndef LASKUTETTAVATMODEL_H
#define LASKUTETTAVATMODEL_H

#include <QAbstractTableModel>
#include <QSet>

class LaskutettavatModel : public QAbstractTableModel
{
    Q_OBJECT
private:
    struct Laskutettava {
        int kumppaniId = 0;
        QString nimi;
        QString osoite;
        QString email;
        QString alvtunnus;
        QString ovttunnus;
        QString valittaja;
        QString kieli;
        int lahetystapa = 0;
    };


public:
    enum { NIMI, KIELI, LAHETYSTAPA };
    enum { LahetysTavatRooli = Qt::UserRole + 2 };


    explicit LaskutettavatModel(QObject *parent = nullptr);

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

    void tallennaLaskut(const QVariantMap& data);
    bool onkoKumppania(int kumppaniId) const;

public slots:
    void lisaa(int kumppaniId);
    void lisaaAsiakas(QVariant* data);
    void poista(int indeksi);

signals:
    void tallennettu();

protected slots:
    void tallennaLasku(const QVariantMap& tallennettava, int indeksi);
    void laskuTallennettu(const QVariantMap& tallennettava, int indeksi, QVariant* vastaus);

private:
    QList<Laskutettava> laskutettavat_;
    QSet<int> kumppaniIdt_;
};

#endif // LASKUTETTAVATMODEL_H
