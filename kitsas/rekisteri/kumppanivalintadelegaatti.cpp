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
#include "kumppanivalintadelegaatti.h"
#include "asiakastoimittajataydentaja.h"

#include "asiakastoimittajalistamodel.h"

#include <QComboBox>
#include <QCompleter>

KumppaniValintaDelegaatti::KumppaniValintaDelegaatti(QWidget *parent) :
    QItemDelegate(parent)
{

}

QWidget *KumppaniValintaDelegaatti::createEditor(QWidget *parent, const QStyleOptionViewItem &/*option*/, const QModelIndex &/*index*/) const
{
    QComboBox *combo = new QComboBox(parent);
    combo->setEditable(true);
    combo->setModel(AsiakasToimittajaListaModel::instanssi());
    combo->completer()->setCompletionMode(QCompleter::PopupCompletion);

    return combo;
}

void KumppaniValintaDelegaatti::setEditorData(QWidget *editor, const QModelIndex &index) const
{
    QComboBox *combo = qobject_cast<QComboBox*>(editor);

    if( index.data(IdRooli).toInt()) {
        combo->setCurrentIndex( combo->findData(index.data(IdRooli)) );
    } else {
        combo->setCurrentText( index.data(Qt::DisplayRole).toString());
    }
}

void KumppaniValintaDelegaatti::setModelData(QWidget *editor, QAbstractItemModel *model, const QModelIndex &index) const
{
    QComboBox *combo = qobject_cast<QComboBox*>(editor);

    QString text = combo->currentText();
    int id = combo->currentData(NimiRooli).toString() == text
              ?  combo->currentData(IdRooli).toInt()
              : 0;



    model->setData(index, id, IdRooli);
    model->setData(index, text, NimiRooli);
}
