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

#ifndef TOSITELAJIMODEL_H
#define TOSITELAJIMODEL_H

#include <QAbstractTableModel>

/**
 * @brief Tositelaji, joka muodostaa oman numerosarjan
 */
class TositeLaji
{
public:
    TositeLaji();
    TositeLaji(int id, QString tunnus, QString nimi);

    int id() const { return id_; }
    QString tunnus() const { return tunnus_; }
    QString nimi() const { return nimi_; }
    bool muokattu() const { return muokattu_; }

    void asetaId(int id);
    void asetaTunnus(const QString& tunnus);
    void asetaNimi(const QString& nimi);
    void nollaaMuokattu();


protected:
    int id_;
    QString tunnus_;
    QString nimi_;
    bool muokattu_;
};


/**
 * @brief Model Tositelajeille
 */

class TositeLajiModel : public QAbstractTableModel
{
    Q_OBJECT
public:
    enum Sarake
    {
        TUNNUS, NIMI
    };

    TositeLajiModel(QObject *parent = 0);

    int rowCount(const QModelIndex &parent) const;
    int columnCount(const QModelIndex &parent) const;

    QVariant headerData(int section, Qt::Orientation orientation, int role) const;
    QVariant data(const QModelIndex &index, int role) const;

    Qt::ItemFlags flags(const QModelIndex &index) const;
    bool setData(const QModelIndex &index, const QVariant &value, int role);

public slots:
    void lataa();
    bool tallenna();
    void lisaaRivi();

protected:
    QList<TositeLaji> lajit_;
};

#endif // TOSITELAJIMODEL_H
