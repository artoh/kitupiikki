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
#ifndef KIELIDELEGAATTI_H
#define KIELIDELEGAATTI_H

#include <QItemDelegate>
#include <QComboBox>

class KieliDelegaatti : public QItemDelegate
{
    Q_OBJECT
public:
    KieliDelegaatti(QObject *parent = nullptr);

    QWidget * createEditor(QWidget *parent, const QStyleOptionViewItem &option, const QModelIndex &index) const override;
    void setEditorData(QWidget *editor, const QModelIndex &index) const override;
    void setModelData(QWidget *editor, QAbstractItemModel *model, const QModelIndex &index) const override;

    static void alustaKieliCombo(QComboBox *combo);
    static QString kieliKoodilla(const QString& kielikoodi);
};

#endif // KIELIDELEGAATTI_H
