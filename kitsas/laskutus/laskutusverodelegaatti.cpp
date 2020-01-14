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

#include "laskutusverodelegaatti.h"

#include <QComboBox>
#include "db/verotyyppimodel.h"
#include "db/kirjanpito.h"
#include "laskualvcombo.h"
#include "laskurivitmodel.h"

LaskutusVeroDelegaatti::LaskutusVeroDelegaatti()
{

}

QWidget *LaskutusVeroDelegaatti::createEditor(QWidget *parent, const QStyleOptionViewItem &/*option*/, const QModelIndex &/*index*/) const
{
    QComboBox *cbox = new LaskuAlvCombo(parent);
    return cbox;
}

void LaskutusVeroDelegaatti::setEditorData(QWidget *editor, const QModelIndex &index) const
{
    int koodi = index.data(LaskuRivitModel::AlvProsenttiRooli).toInt() * 100 + index.data(LaskuRivitModel::AlvKoodiRooli).toInt();
    if( index.data(LaskuRivitModel::VoittomarginaaliRooli).toInt())
        koodi = index.data(LaskuRivitModel::VoittomarginaaliRooli).toInt();

    LaskuAlvCombo *cbox = qobject_cast<LaskuAlvCombo*>(editor);
    cbox->setCurrentIndex( cbox->findData(koodi) );
}

void LaskutusVeroDelegaatti::setModelData(QWidget *editor, QAbstractItemModel *model, const QModelIndex &index) const
{
    LaskuAlvCombo *cbox = qobject_cast<LaskuAlvCombo*>(editor);
    int koodi = cbox->currentData().toInt();

    if( koodi >= LaskuRivitModel::Kaytetyt)
    {
        model->setData(index, koodi, LaskuRivitModel::VoittomarginaaliRooli);
    }
    else
    {
        model->setData(index, koodi / 100, LaskuRivitModel::AlvProsenttiRooli);
        model->setData(index, koodi % 100, LaskuRivitModel::AlvKoodiRooli);
    }

}


