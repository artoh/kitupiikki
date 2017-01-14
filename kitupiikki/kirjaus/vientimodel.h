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

#ifndef VIENTIMODEL_H
#define VIENTIMODEL_H

#include <QAbstractTableModel>
#include <QDate>
#include "db/tili.h"
#include <QList>

class Kirjanpito;
class KirjausWg;

/**
 * @brief Yhden viennin tiedot
 */
struct VientiRivi
{
    int vientiId = 0;
    QDate pvm;
    Tili tili;
    QString selite;
    int debetSnt = 0;
    int kreditSnt = 0;
};


class VientiModel : public QAbstractTableModel
{
    Q_OBJECT
public:

    enum VientiSarake
    {
        PVM, TILI, DEBET, KREDIT, SELITE, KUSTANNUSPAIKKA, PROJEKTI
    };


    VientiModel(Kirjanpito *kp, KirjausWg *kwg);

    int rowCount(const QModelIndex &parent) const;
    int columnCount(const QModelIndex &parent) const;
    QVariant headerData(int section, Qt::Orientation orientation, int role) const;
    QVariant data(const QModelIndex &index, int role) const;
    bool setData(const QModelIndex &index, const QVariant &value, int role);
    Qt::ItemFlags flags(const QModelIndex &index) const;
    bool insertRows(int row, int count, const QModelIndex &);
    bool lisaaRivi();

public slots:
    void tallenna(int tositeid);

signals:
    void siirrySarakkeeseen(QModelIndex index);

protected:
    QList<VientiRivi> viennit;
    Kirjanpito *kirjanpito;
    KirjausWg *kirjauswg;

    VientiRivi uusiRivi();
};

#endif // VIENTIMODEL_H
