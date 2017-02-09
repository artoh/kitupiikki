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

#include <QCompleter>
#include <QLineEdit>

#include <QSortFilterProxyModel>

#include "tilidelegaatti.h"

#include "db/kirjanpito.h"
#include "vientimodel.h"

TiliDelegaatti::TiliDelegaatti()
{

}

QWidget *TiliDelegaatti::createEditor(QWidget *parent, const QStyleOptionViewItem & /* option */, const QModelIndex & /* index */) const
{
    QLineEdit *editor = new QLineEdit(parent);

    // Tilivalintaan TiliModelista tilit (taso 0)

    QCompleter *taydennin = new QCompleter() ;

    QSortFilterProxyModel *proxy = new QSortFilterProxyModel(parent);
    proxy->setSourceModel( Kirjanpito::db()->tilit() );
    proxy->setFilterRole( TiliModel::OtsikkotasoRooli);
    proxy->setFilterFixedString("0");

    taydennin->setCompletionColumn( TiliModel::NRONIMI);
    taydennin->setModel( proxy );

    taydennin->setCompletionMode( QCompleter::UnfilteredPopupCompletion);
    editor->setCompleter(taydennin);

    return editor;
}

void TiliDelegaatti::setEditorData(QWidget *editor, const QModelIndex &index) const
{
    QLineEdit *lineeditor = qobject_cast<QLineEdit*>(editor);
    lineeditor->setText( index.data().toString());
}

void TiliDelegaatti::setModelData(QWidget *editor, QAbstractItemModel *model, const QModelIndex &index) const
{
    QLineEdit *lineeditor = qobject_cast<QLineEdit*>(editor);
    QString tekstina = lineeditor->text();
    QString sana = tekstina.left( tekstina.indexOf(' ') );

    // Sitten pitää etsiä tiliä tililistalta
    // Täydentää seuraavaan sopivaan
    for(int i=0; i < kp()->tilit()->rowCount(QModelIndex()); i++)
    {
        Tili tili = kp()->tilit()->tiliIndeksilla(i);

        QString numeroTeksti = QString::number(tili.numero());
        if( numeroTeksti.startsWith(sana))
        {
            model->setData(index, QVariant( tili.numero()));
            return;
        }
    }

}




