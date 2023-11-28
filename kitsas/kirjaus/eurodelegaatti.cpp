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


#include "tools/kpeuroedit.h"

EuroDelegaatti::EuroDelegaatti(QObject *parent) :
    QItemDelegate (parent)
{

}

QWidget *EuroDelegaatti::createEditor(QWidget *parent, const QStyleOptionViewItem & /* option */, const QModelIndex & /* index */) const
{
    KpEuroEdit *edit = new KpEuroEdit(parent);
    return edit;
}

void EuroDelegaatti::setEditorData(QWidget *editor, const QModelIndex &index) const
{
    KpEuroEdit *edit = qobject_cast<KpEuroEdit*>(editor);
    const Euro euro(index.data(Qt::EditRole).toString());
    edit->setEuro( euro );
    if( !euro) {
        edit->setMiinus( index.data(MiinusRooli).toBool() );
    }

}

void EuroDelegaatti::setModelData(QWidget *editor, QAbstractItemModel *model, const QModelIndex &index) const
{
    KpEuroEdit *edit = qobject_cast<KpEuroEdit*>(editor);
    model->setData(index, edit->value() );

}
