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
public:
    class Laskutettava {
    public:
        Laskutettava();
        Laskutettava(const QVariantMap& map);

        QString kieli() const;
        void setKieli(const QString &kieli);
        int lahetystapa() const;
        void setLahetystapa(const int lahetystapa);

        QVariantList lahetystavat() const;
        QString nimi() const;
        QString email() const;
        QString osoite() const;
        QVariantMap map() const;
        int id() const;

    protected:
        QString kieli_;
        int lahetystapa_;
        QVariantMap kumppani_;

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

    bool onkoKumppania(int kumppaniId) const;

    QList<Laskutettava> laskutettavat() const;

public slots:
    void lisaa(int kumppaniId);
    void lisaaAsiakas(QVariant* data);
    void poista(int indeksi);

signals:
    void tallennettu();

private:
    QList<Laskutettava> laskutettavat_;
    QSet<int> kumppaniIdt_;
};

#endif // LASKUTETTAVATMODEL_H
