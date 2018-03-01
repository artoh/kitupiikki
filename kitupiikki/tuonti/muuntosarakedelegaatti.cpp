/*
   Copyright (C) 2018 Arto Hyvättinen

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

#include "muuntosarakedelegaatti.h"

#include "db/tilinvalintaline.h"
#include "db/kirjanpito.h"


MuuntoSarakeDelegaatti::MuuntoSarakeDelegaatti(QObject *parent)
    : QItemDelegate( parent)
{

}

void MuuntoSarakeDelegaatti::paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    Tili tili = kp()->tilit()->tiliNumerolla( index.data().toInt() );
    QString tekstina;

    if( tili.onkoValidi())
        tekstina= QString("%1 %2").arg(tili.numero()).arg(tili.nimi());

    drawDisplay(painter, option, option.rect, tekstina);
    drawFocus(painter, option, option.rect);

}

QWidget *MuuntoSarakeDelegaatti::createEditor(QWidget *parent, const QStyleOptionViewItem & /* option */, const QModelIndex & /* index */) const
{
    TilinvalintaLineDelegaatille *edit = new TilinvalintaLineDelegaatille(parent);
    return edit;
}

void MuuntoSarakeDelegaatti::setEditorData(QWidget *editor, const QModelIndex &index) const
{
    TilinvalintaLineDelegaatille *edit = qobject_cast<TilinvalintaLineDelegaatille*>(editor);
    edit->valitseTiliNumerolla( index.data(Qt::EditRole).toInt() );
}

void MuuntoSarakeDelegaatti::setModelData(QWidget *editor, QAbstractItemModel *model, const QModelIndex &index) const
{
    TilinvalintaLineDelegaatille *edit = qobject_cast<TilinvalintaLineDelegaatille*>(editor);
    model->setData(index , edit->valittuTilinumero(), Qt::EditRole);
}
