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

#include <QComboBox>
#include "db/kirjanpito.h"

#include <QDebug>

#include "kohdennusdelegaatti.h"

KohdennusDelegaatti::KohdennusDelegaatti()
{

}

QWidget *KohdennusDelegaatti::createEditor(QWidget *parent, const QStyleOptionViewItem & /* option */, const QModelIndex &/*index*/) const
{
    QComboBox *cbox = new QComboBox(parent);
    cbox->setModel( kp()->kohdennukset());
    cbox->setModelColumn( KohdennusModel::NIMI);
    return cbox;
}

void KohdennusDelegaatti::setEditorData(QWidget *editor, const QModelIndex &index) const
{
    QComboBox *cbox = qobject_cast<QComboBox*>(editor);
    cbox->setCurrentIndex( cbox->findData( index.data( Qt::EditRole).toInt(), KohdennusModel::IdRooli ));
}

void KohdennusDelegaatti::setModelData(QWidget *editor, QAbstractItemModel *model, const QModelIndex &index) const
{
    QComboBox *cbox = qobject_cast<QComboBox*>(editor);
    model->setData( index, cbox->currentData(KohdennusModel::IdRooli).toInt(), Qt::EditRole);
}
