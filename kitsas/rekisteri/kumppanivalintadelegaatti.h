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
#ifndef KUMPPANIVALINTADELEGAATTI_H
#define KUMPPANIVALINTADELEGAATTI_H

#include <QItemDelegate>

class AsiakasToimittajaTaydentaja;

class KumppaniValintaDelegaatti : public QItemDelegate
{
public:

    enum {
        NimiRooli = Qt::DisplayRole,
        IdRooli = Qt::UserRole
    };

    KumppaniValintaDelegaatti(QWidget *parent = nullptr);

    QWidget * createEditor(QWidget *parent, const QStyleOptionViewItem &option, const QModelIndex &index) const override;
    void setEditorData(QWidget *editor, const QModelIndex &index) const override;
    void setModelData(QWidget *editor, QAbstractItemModel *model, const QModelIndex &index) const override;

};

#endif // KUMPPANIVALINTADELEGAATTI_H
