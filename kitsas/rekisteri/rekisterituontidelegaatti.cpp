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
#include "rekisterituontidelegaatti.h"
#include <QComboBox>
#include "rekisterituontimodel.h"

RekisteriTuontiDelegaatti::RekisteriTuontiDelegaatti(QObject *parent)
    : QItemDelegate(parent)
{

}

QWidget *RekisteriTuontiDelegaatti::createEditor(QWidget *parent, const QStyleOptionViewItem & /*option*/, const QModelIndex &/*index*/) const
{
    QComboBox *combo = new QComboBox(parent);

    for(int i=0; i<=RekisteriTuontiModel::LISATIETO; i++)
        combo->addItem( RekisteriTuontiModel::otsikkoTeksti(i), i );

    return combo;
}

void RekisteriTuontiDelegaatti::setEditorData(QWidget *editor, const QModelIndex &index) const
{
    QComboBox *combo = qobject_cast<QComboBox*>(editor);
    combo->setCurrentIndex( combo->findData( index.data() ) );
}

void RekisteriTuontiDelegaatti::setModelData(QWidget *editor, QAbstractItemModel *model, const QModelIndex &index) const
{
    QComboBox *combo = qobject_cast<QComboBox*>(editor);
    model->setData(index, combo->currentData());
}



