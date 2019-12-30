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
#ifndef TOIMITUSTAPADELEGAATTI_H
#define TOIMITUSTAPADELEGAATTI_H

#include <QItemDelegate>

class QComboBox;

class ToimitustapaDelegaatti : public QItemDelegate
{
    Q_OBJECT
public:
    ToimitustapaDelegaatti(QObject *parent = nullptr);

    QWidget * createEditor(QWidget *parent, const QStyleOptionViewItem &option, const QModelIndex &index) const override;
    void setEditorData(QWidget *editor, const QModelIndex &index) const override;
    void setModelData(QWidget *editor, QAbstractItemModel *model, const QModelIndex &index) const override;

    static QString toimitustapa(int koodi);
    static QIcon icon(int koodi);
    static void alustaCombobox(QComboBox* combo, const QVariantList& tavat);
};

#endif // TOIMITUSTAPADELEGAATTI_H
