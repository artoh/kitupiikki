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
#include "yksikkodelegaatti.h"

#include "yksikkocombo.h"
#include "model/tositerivit.h"

#include <QDebug>

YksikkoDelegaatti::YksikkoDelegaatti(QObject *parent)
    : QItemDelegate(parent)
{

}

QWidget *YksikkoDelegaatti::createEditor(QWidget *parent, const QStyleOptionViewItem & /*option*/, const QModelIndex &index) const
{
    YksikkoCombo *combo = new YksikkoCombo(parent,
                                           index.data( TositeRivit::UNkoodiRooli ).toString().isEmpty());
    return combo;
}

void YksikkoDelegaatti::setEditorData(QWidget *editor, const QModelIndex &index) const
{
    YksikkoCombo *combo = qobject_cast<YksikkoCombo*>(editor);
    qDebug() << " UN MODEL " << index.data( TositeRivit::UNkoodiRooli ).toString()
             << " - " << index.data(Qt::EditRole).toString();

    if( !index.data( TositeRivit::UNkoodiRooli ).toString().isEmpty()) {
        combo->setUNkoodi( index.data(TositeRivit::UNkoodiRooli).toString() );
    } else if( !index.data( Qt::EditRole).toString().isEmpty()) {
        combo->setYksikko( index.data(Qt::EditRole).toString() );
    }
}

void YksikkoDelegaatti::setModelData(QWidget *editor, QAbstractItemModel *model, const QModelIndex &index) const
{
    YksikkoCombo *combo = qobject_cast<YksikkoCombo*>(editor);
    if( !combo->unKoodi().isEmpty()) {
        model->setData(index, combo->unKoodi(), TositeRivit::UNkoodiRooli);
    } else if( !combo->yksikko().isEmpty()) {
        model->setData(index, combo->yksikko(), Qt::EditRole );
    }
}
