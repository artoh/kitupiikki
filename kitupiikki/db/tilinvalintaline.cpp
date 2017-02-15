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

#include "tilinvalintaline.h"
#include "tilinvalintadialogi.h"

#include <QCompleter>
#include <QSortFilterProxyModel>
#include <QDebug>
#include <QKeyEvent>

TilinvalintaLineDelegaatille::TilinvalintaLineDelegaatille(QWidget *parent)
    : QLineEdit(parent)
{
    QCompleter *taydennin = new QCompleter() ;

    QSortFilterProxyModel *proxy = new QSortFilterProxyModel(parent);
    proxy->setSourceModel( Kirjanpito::db()->tilit() );
    proxy->setFilterRole( TiliModel::OtsikkotasoRooli);
    proxy->setFilterFixedString("0");

    taydennin->setCompletionColumn( TiliModel::NRONIMI);
    taydennin->setModel( proxy );

    taydennin->setCompletionMode( QCompleter::UnfilteredPopupCompletion);
    setCompleter(taydennin);

    connect(this, SIGNAL(tiliValittu(Tili)), this, SLOT(valitseTili(Tili)));
}

void TilinvalintaLineDelegaatille::valitseTiliNumerolla(int tilinumero)
{
    if( tilinumero )
    {
        Tili tili = kp()->tilit()->tiliNumerolla(tilinumero);
        setText( tr("%1 %2").arg(tili.numero()).arg(tili.nimi()));
    }

}


int TilinvalintaLineDelegaatille::valittuTilinumero() const
{
    QString sana = text().left( text().indexOf(' '));
    if( sana.isEmpty() || !sana.at(0).isDigit() )
        return 0;

    for( int i=0; i < kp()->tilit()->rowCount( QModelIndex()); i++)
    {
        Tili tili = kp()->tilit()->tiliIndeksilla(i);
        if( !tili.otsikkotaso())
        {
            QString nroTxt = QString::number( tili.numero() );
            if( nroTxt.startsWith(sana))
            {
                return tili.numero();
            }
        }
    }
    return 0;
}

void TilinvalintaLineDelegaatille::keyPressEvent(QKeyEvent *event)
{
    if( event->text().at(0).isLetter()
            || event->key() == Qt::Key_Space)
    {
        alku_ = event->text();
        qobject_cast<QWidget*>(parent())->setFocus();
    }
    else
        QLineEdit::keyPressEvent(event);
}

void TilinvalintaLineDelegaatille::valitseTili(Tili tili)
{
    setText( tr("%1 %2").arg(tili.numero()).arg(tili.nimi()));
}


