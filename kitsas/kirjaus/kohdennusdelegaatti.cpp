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

#include "db/kirjanpito.h"

#include <QDebug>

#include "tools/kohdennuscombo.h"

#include "kohdennusdelegaatti.h"
#include "model/tositeviennit.h"
#include "db/kirjanpito.h"


KohdennusDelegaatti::KohdennusDelegaatti(QObject *parent) :
    QItemDelegate(parent),
    model( new KohdennusProxyModel(this))
{

}

QWidget *KohdennusDelegaatti::createEditor(QWidget *parent, const QStyleOptionViewItem & /* option */, const QModelIndex &index) const
{
    Tili* tili = kp()->tilit()->tili( index.data(TositeViennit::TiliNumeroRooli).toInt() );
    if( tili && ( tili->onko(TiliLaji::TULOS) || tili->onko(TiliLaji::POISTETTAVA))) {
        KohdennusCombo* cbox = new KohdennusCombo(parent);
        return cbox;
    }
    return nullptr;

}

void KohdennusDelegaatti::setEditorData(QWidget *editor, const QModelIndex &index) const
{

    KohdennusCombo* cbox = qobject_cast<KohdennusCombo*>(editor);

    if( kohdennusPaiva.isValid())
        cbox->suodataPaivalla( kohdennusPaiva );
    else
        cbox->suodataPaivalla(index.data( TositeViennit::PvmRooli ).toDate() );

    cbox->valitseKohdennus( index.data( Qt::EditRole).toInt() );

}

void KohdennusDelegaatti::setModelData(QWidget *editor, QAbstractItemModel *model, const QModelIndex &index) const
{
    KohdennusCombo *cbox = qobject_cast<KohdennusCombo*>(editor);
    model->setData( index, cbox->kohdennus(), Qt::EditRole);

}

void KohdennusDelegaatti::asetaKohdennusPaiva(const QDate &paiva)
{
    kohdennusPaiva = paiva;
}
