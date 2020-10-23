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
#include "tilioteview.h"

#include "kirjaus/tilidelegaatti.h"
#include "kirjaus/eurodelegaatti.h"
#include "kirjaus/kohdennusdelegaatti.h"

#include "rekisteri/kumppanivalintadelegaatti.h"

#include "tiliotemodel.h"
#include "db/kirjanpito.h"

#include <QHeaderView>
#include <QSettings>
#include <QDebug>
#include <QTimer>
#include <QKeyEvent>

TilioteView::TilioteView(QWidget *parent) :
    QTableView(parent)
{
    setItemDelegateForColumn( TilioteModel::TILI, new TiliDelegaatti(this) );
    setItemDelegateForColumn( TilioteModel::EURO, new EuroDelegaatti(this) );
    setItemDelegateForColumn( TilioteModel::KOHDENNUS, new KohdennusDelegaatti(this) );

    setItemDelegateForColumn(TilioteModel::SAAJAMAKSAJA,
                             new KumppaniValintaDelegaatti(this));

    sortByColumn(TilioteModel::PVM, Qt::AscendingOrder);

    setFocusPolicy(Qt::StrongFocus);
    horizontalHeader()->setStretchLastSection(true);

    QTimer::singleShot(10, [this] {this->horizontalHeader()->restoreState(kp()->settings()->value("TilioteRuudukko").toByteArray());});

}

TilioteView::~TilioteView()
{
    kp()->settings()->setValue("TilioteRuudukko", horizontalHeader()->saveState());
}

void TilioteView::keyPressEvent(QKeyEvent *event)
{
    if( event->key() == Qt::Key_Insert) {
        model()->insertRow(currentIndex().row()+1);
        setCurrentIndex(currentIndex().sibling(currentIndex().row()+1, TilioteModel::PVM));
        return;
    } else if( ( event->key() == Qt::Key_Enter ||
        event->key() == Qt::Key_Return ||
        event->key() == Qt::Key_Insert ||
        event->key() == Qt::Key_Tab) &&
            event->modifiers() == Qt::NoModifier )  {

        if( currentIndex().column() == TilioteModel::EURO &&
            currentIndex().row() == model()->rowCount() - 1) {
            model()->insertRow(currentIndex().row()+1);
            setCurrentIndex(currentIndex().sibling(currentIndex().row()+1, TilioteModel::PVM));
            return;
        } else if( event->key() == Qt::Key_Enter || event->key() == Qt::Key_Return) {
            if(currentIndex().row() == model()->rowCount() - 1 && qAbs(currentIndex().sibling(currentIndex().row(), TilioteModel::EURO).data(Qt::EditRole).toDouble()) > 1e-5) {
                model()->insertRow(currentIndex().row()+1);
            }
            int uusirivi = currentIndex().row() + 1;
            while(currentIndex().sibling(uusirivi,0).isValid() && (currentIndex().sibling(uusirivi,0).flags() & Qt::ItemIsEnabled) == 0) {
                uusirivi++;
            }
            setCurrentIndex(currentIndex().sibling(uusirivi, currentIndex().column()));
        }

    }
    QTableView::keyPressEvent(event);
}

void TilioteView::closeEditor(QWidget *editor, QAbstractItemDelegate::EndEditHint hint)
{
    QTableView::closeEditor(editor, QAbstractItemDelegate::NoHint);
    if( hint == QAbstractItemDelegate::EditNextItem) {
        if( currentIndex().column() == TilioteModel::EURO) {
            if( currentIndex().row() == model()->rowCount() + 1) {
                model()->insertRow(currentIndex().row() + 1);
            }
            setCurrentIndex(currentIndex().sibling(currentIndex().row() + 1, TilioteModel::PVM));
        } else {
            setCurrentIndex(currentIndex().sibling(currentIndex().row(), currentIndex().column() + 1));
        }

    } else if( hint == QAbstractItemDelegate::EditPreviousItem) {
        if( currentIndex().column() > 0)
            setCurrentIndex( model()->index( currentIndex().row(), currentIndex().column()-1 ) );
        else if( currentIndex().row() > 0)
            setCurrentIndex(model()->index( currentIndex().row()-1, TilioteModel::EURO));
    }

}

void TilioteView::currentChanged(const QModelIndex &current, const QModelIndex &previous)
{

    qDebug() << previous.row() << "--->" << current.row() << " @ " << lastValidIndex_.row();

    if(!previous.isValid() && lastValidIndex_.isValid() && current.row() == 0 && current.column() == 0 && lastValidIndex_.column() == TilioteModel::TILI) {
        setCurrentIndex(current.sibling(lastValidIndex_.row(), lastValidIndex_.column()));
    } else {
        if(current.isValid()) {
            lastValidIndex_ = current;
        }
        QTableView::currentChanged(current, previous);
    }
}



