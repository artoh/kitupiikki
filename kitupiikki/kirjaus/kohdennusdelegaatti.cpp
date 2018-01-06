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
#include "kohdennusproxymodel.h"

KohdennusDelegaatti::KohdennusDelegaatti(QObject *parent) :
    QItemDelegate(parent),
    model( new KohdennusProxyModel(this))
{

}

QWidget *KohdennusDelegaatti::createEditor(QWidget *parent, const QStyleOptionViewItem & /* option */, const QModelIndex &/*index*/) const
{
    QComboBox *cbox = new QComboBox(parent);
    cbox->setModel( model );
    cbox->setModelColumn( KohdennusModel::NIMI);
    return cbox;
}

void KohdennusDelegaatti::setEditorData(QWidget *editor, const QModelIndex &index) const
{
    QComboBox *cbox = qobject_cast<QComboBox*>(editor);
    model->asetaKohdennus( index.data( Qt::EditRole).toInt() );

    if( kohdennusPaiva.isValid())
        model->asetaPaiva( kohdennusPaiva);
    else
        model->asetaPaiva( index.data( VientiModel::PvmRooli ).toDate() );

    cbox->setCurrentIndex( cbox->findData( index.data( Qt::EditRole).toInt(), KohdennusModel::IdRooli ));
}

void KohdennusDelegaatti::setModelData(QWidget *editor, QAbstractItemModel *model, const QModelIndex &index) const
{
    QComboBox *cbox = qobject_cast<QComboBox*>(editor);
    model->setData( index, cbox->currentData(KohdennusModel::IdRooli).toInt(), Qt::EditRole);
}

void KohdennusDelegaatti::asetaKohdennusPaiva(const QDate &paiva)
{
    kohdennusPaiva = paiva;
}
