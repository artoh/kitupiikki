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

PvmDelegaatti::PvmDelegaatti(QDateEdit *kantapaivaeditori) :
    kantaeditori(kantapaivaeditori)
{

}

QWidget *PvmDelegaatti::createEditor(QWidget *parent, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    QDateEdit *edit = new QDateEdit(parent);

    Tilikausi tositekausi = Kirjanpito::db()->tilikausiPaivalle(kantaeditori->date());
    edit->setDateRange( tositekausi.alkaa(), tositekausi.paattyy());

    return edit;
}

void PvmDelegaatti::setEditorData(QWidget *editor, const QModelIndex &index) const
{
    QDateEdit *edit = qobject_cast<QDateEdit*>(editor);
    edit->setDate( index.data(Qt::EditRole).toDate());
}

void PvmDelegaatti::setModelData(QWidget *editor, QAbstractItemModel *model, const QModelIndex &index) const
{
    QDateEdit *edit = qobject_cast<QDateEdit*>(editor);
    model->setData(index, edit->date());
}


