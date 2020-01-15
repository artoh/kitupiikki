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

#include "pvmdelegaatti.h"
#include "db/kirjanpito.h"
#include "db/tilikausi.h"

#include "tools/kpdateedit.h"

PvmDelegaatti::PvmDelegaatti(KpDateEdit *kantapaivaeditori, QObject *parent) :
    QItemDelegate(parent),
    kantaeditori(kantapaivaeditori)
{

}

QWidget *PvmDelegaatti::createEditor(QWidget *parent, const QStyleOptionViewItem & /* option */, const QModelIndex & /* index */ ) const
{
    KpDateEdit *edit = new KpDateEdit(parent);
    return edit;
}

void PvmDelegaatti::setEditorData(QWidget *editor, const QModelIndex &index) const
{
    KpDateEdit *edit = qobject_cast<KpDateEdit*>(editor);
    edit->setDate( index.data(Qt::EditRole).toDate());
}

void PvmDelegaatti::setModelData(QWidget *editor, QAbstractItemModel *model, const QModelIndex &index) const
{
    KpDateEdit *edit = qobject_cast<KpDateEdit*>(editor);
    model->setData(index, edit->date());
}


