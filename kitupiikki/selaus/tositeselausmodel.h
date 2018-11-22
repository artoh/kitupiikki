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

#ifndef TOSITESELAUSMODEL_H
#define TOSITESELAUSMODEL_H

#include <QAbstractTableModel>
#include <QDate>
#include <QList>

/**
 * @brief Yhden tositteen tiedot tositteiden selauksessa
 */
struct TositeSelausRivi
{
    int tositeId;
    QDate pvm;
    int tositeLaji;
    int tositeTunniste;

    QString otsikko;
    qlonglong summa;

    bool liitteita;

};

/**
 * @brief Tositteiden selauksen model
 */
class TositeSelausModel : public QAbstractTableModel
{
public:
    enum Sarake
    {
        TUNNISTE, PVM, TOSITELAJI, SUMMA, OTSIKKO
    };

    enum
    {
        TositeIdRooli = Qt::UserRole,
        TositeLajiIdRooli = Qt::UserRole + 1,
    };

    TositeSelausModel();

    int rowCount(const QModelIndex &parent) const;
    int columnCount(const QModelIndex &parent) const;

    QVariant headerData(int section, Qt::Orientation orientation, int role) const;
    QVariant data(const QModelIndex &index, int role) const;

    QStringList lajiLista() const { return kaytetytLajinimet; }

public slots:
    void lataa(const QDate& alkaa, const QDate& loppuu);

protected:
    QList<TositeSelausRivi> rivit;
    QStringList kaytetytLajinimet;

};

#endif // TOSITESELAUSMODEL_H
