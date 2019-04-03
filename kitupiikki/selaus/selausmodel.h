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

#ifndef SELAUSMODEL_H
#define SELAUSMODEL_H

#include <QAbstractTableModel>
#include <QList>
#include <QDate>

#include "db/tili.h"
#include "db/kohdennus.h"
#include "db/eranvalintamodel.h"

/**
 * @brief SelausModel:in yhden rivin (viennin) tiedot
 */
struct SelausRivi
{
    int tositeId;
    QDate pvm;
    Tili tili;
    Kohdennus kohdennus;

    QString selite;
    qlonglong debetSnt;
    qlonglong kreditSnt;
    TaseEra taseEra;
    QString tositetunniste;
    QString lajiteltavaTositetunniste;
    QStringList tagit;
    bool eraMaksettu = false;
    int vientiId;
    bool liitteita = false;
};

/**
 * @brief Selaussivun model vientien selaamiseen
 */
class SelausModel : public QAbstractTableModel
{
    Q_OBJECT
public:

    enum SelausSarake
    {
        TOSITE, PVM, TILI, DEBET, KREDIT, KOHDENNUS, SELITE
    };

    SelausModel();

    int rowCount(const QModelIndex &parent) const;
    int columnCount(const QModelIndex &parent) const;

    QVariant headerData(int section, Qt::Orientation orientation, int role) const;
    QVariant data(const QModelIndex &index, int role) const;

    QStringList kaytetytTilit() const { return tileilla; }

public slots:
    void lataa(const QDate& alkaa, const QDate& loppuu);

    void tietoSaapuu(QVariantMap *map, int status);

protected:
    QList<SelausRivi> rivit;
    QStringList tileilla;

    QVariantList lista_;

};

#endif // SELAUSMODEL_H
