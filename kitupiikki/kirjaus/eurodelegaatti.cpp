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

#include "eurodelegaatti.h"

#include <QDoubleSpinBox>

EuroDelegaatti::EuroDelegaatti()
{

}

QWidget *EuroDelegaatti::createEditor(QWidget *parent, const QStyleOptionViewItem & /* option */, const QModelIndex & /* index */) const
{
    QDoubleSpinBox *sbox = new QDoubleSpinBox(parent);
    sbox->setSuffix("€");
    sbox->setDecimals(2);
    sbox->setRange(-999999999.99,999999999.99);
    return sbox;
}

void EuroDelegaatti::setEditorData(QWidget *editor, const QModelIndex &index) const
{
    QDoubleSpinBox *sbox = qobject_cast<QDoubleSpinBox*>(editor);
    sbox->setValue( index.data(Qt::EditRole).toDouble() / 100.0 );
}

void EuroDelegaatti::setModelData(QWidget *editor, QAbstractItemModel *model, const QModelIndex &index) const
{
    QDoubleSpinBox *sbox = qobject_cast<QDoubleSpinBox*>(editor);
    model->setData(index, sbox->value() );

}
