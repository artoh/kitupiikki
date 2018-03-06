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

#ifndef ALVILMOITUSTENMODEL_H
#define ALVILMOITUSTENMODEL_H

#include <QDate>
#include <QAbstractTableModel>
#include <QList>

/**
 * @brief Yhden alv-ilmoituksen tiedot
 */
struct AlvIlmoitusTieto
{
    AlvIlmoitusTieto() {}
    int tositeId;
    QDate alkuPvm;
    QDate loppuPvm;
    int maksettavaVeroSnt;
};

/**
 * @brief Tehtyjen arvonlisäilmoitusten model
 */
class AlvIlmoitustenModel : public QAbstractTableModel
{
    Q_OBJECT
public:
    enum Sarake
    {
        ALKAA, PAATTYY, ERAPVM, VEROSNT
    };
    enum
    {
        TositeIdRooli = Qt::UserRole
    };

    AlvIlmoitustenModel(QObject *parent = 0);

    int rowCount(const QModelIndex &parent) const;
    int columnCount(const QModelIndex &parent) const;

    QVariant headerData(int section, Qt::Orientation orientation, int role) const;
    QVariant data(const QModelIndex &index, int role) const;

public slots:
    void lataa();

protected:
    QList<AlvIlmoitusTieto> tiedot_;

};

#endif // ALVILMOITUSTENMODEL_H
