/*
   Copyright (C) 2017 Arto Hyvättinen

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

#ifndef KUSTANNUSPAIKKAMODEL_H
#define KUSTANNUSPAIKKAMODEL_H

#include <QAbstractTableModel>
#include <QList>


/**
 * @brief The Kustannuspaikan tiedot
 */
class Kustannuspaikka
{
public:
    Kustannuspaikka(const QString kpnimi = QString());
    Kustannuspaikka(int id, const QString kpnimi);

    int id() const { return id_; }
    QString nimi() const { return nimi_; }

    void asetaId(int id) { id_ = id; muokattu_ = true; }
    void asetaNimi(const QString& kpnimi) { nimi_ = kpnimi; muokattu_ = true; }

    bool muokattu() const { return muokattu_; }
    void nollaaMuokattu() { muokattu_ = false; }

protected:
    int id_;
    QString nimi_;
    bool muokattu_;
};


/**
 * @brief Kustannuspaikkojen luettelo
 *
 * Luettelo kustannuspaikoista, numero on IdRooli:ssa
 *
 */
class KustannuspaikkaModel : public QAbstractTableModel
{
    Q_OBJECT
public:

    enum
    {
        IdRooli = Qt::UserRole + 1
    };

    enum Sarake
    {
        NIMI
    };

    KustannuspaikkaModel(QObject *parent = 0);

    int rowCount(const QModelIndex &parent) const;
    int columnCount(const QModelIndex &parent) const;

    QVariant headerData(int section, Qt::Orientation orientation, int role) const;
    QVariant data(const QModelIndex &index, int role) const;

    Qt::ItemFlags flags(const QModelIndex &index) const;
    bool setData(const QModelIndex &index, const QVariant &value, int role);

    QString nimi(int id) const;
    Kustannuspaikka kustannuspaikka(int id) const;
    QList<Kustannuspaikka> kustannuspaikat() const;

public slots:
    void lataa();
    void lisaaUusi(const QString& nimi = QString());

protected:
    QList<Kustannuspaikka> kustannuspaikat_;

};

#endif // KUSTANNUSPAIKKAMODEL_H
