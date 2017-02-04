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

#include "tilidelegaatti.h"

#include "db/kirjanpito.h"
#include "vientimodel.h"

TiliDelegaatti::TiliDelegaatti()
{

}

QWidget *TiliDelegaatti::createEditor(QWidget *parent, const QStyleOptionViewItem & /* option */, const QModelIndex & /* index */) const
{
    QLineEdit *editor = new QLineEdit(parent);

    QList<Tili> tilit= Kirjanpito::db()->tilit();

    QStringList tililista;

    foreach (Tili tili, tilit)
    {
        tililista.append( QString("%1 %2").arg(tili.numero()).arg(tili.nimi()) );
    }

    QCompleter *taydennin = new QCompleter( tililista);
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
    QList<Tili> tilit = Kirjanpito::db()->tilit();
    foreach (Tili tili, tilit)
    {
        QString numeroTeksti = QString::number(tili.numero());
        if( numeroTeksti.startsWith(sana))
        {
            model->setData(index, QVariant( tili.id()));
            return;
        }
    }

}




