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
#ifndef TOSITESELAUSPROXYMODEL_H
#define TOSITESELAUSPROXYMODEL_H

#include <QSortFilterProxyModel>

#include "tositeselausmodel.h"

class TositeSelausProxyModel : public QSortFilterProxyModel
{
public:
    TositeSelausProxyModel(TositeSelausModel* model, QObject* parent);

    void etsi(const QString& teksti);
    void suodataTositetyyppi(int tyyppi);
    void suodataTositesarja(const QString& sarja);
    void suodataHuomio(bool onko);

protected:
    virtual bool filterAcceptsRow(int source_row, const QModelIndex& source_parent) const override;

protected:
    TositeSelausModel* model_;

    int tyyppiSuodatus_ = -1;
    QString sarjaSuodatus_;
    QString etsiSuodatus_;
    bool huomioSuodatus_ = false;


};

#endif // TOSITESELAUSPROXYMODEL_H
