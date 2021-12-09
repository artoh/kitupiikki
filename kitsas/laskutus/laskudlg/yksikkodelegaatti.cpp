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

    const QString unKoodi = index.data(TositeRivit::UNkoodiRooli).toString();
    const QString yksikko = index.data(Qt::EditRole).toString();


    if( unKoodi.isEmpty() ) {
        combo->setYksikko( yksikko);
    } else {
        combo->setUNkoodi(unKoodi);
    }

}

void YksikkoDelegaatti::setModelData(QWidget *editor, QAbstractItemModel *model, const QModelIndex &index) const
{
    YksikkoCombo *combo = qobject_cast<YksikkoCombo*>(editor);

    const QString unKoodi = combo->unKoodi();
    const QString yksikko = combo->yksikko();

    if( unKoodi.isEmpty()) {
        model->setData(index, yksikko, Qt::EditRole );
    } else {
        model->setData(index, unKoodi, TositeRivit::UNkoodiRooli);
    }
}
