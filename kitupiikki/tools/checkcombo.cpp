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
#include "checkcombo.h"
#include <QLineEdit>
#include <QStandardItem>
#include <QStandardItemModel>
#include <QEvent>
#include <QListView>
#include <QFocusEvent>
#include <QDebug>

#include "kirjaus/kohdennusproxymodel.h"
#include "db/kohdennusmodel.h"

CheckCombo::CheckCombo(QWidget *parent) :
    QComboBox (parent),
    model_( new QStandardItemModel(this))
{
    setEditable(true);
    lineEdit()->setReadOnly(true);
    lineEdit()->installEventFilter(this);
    setItemDelegate( new CheckCombo::CheckComboStyledItemDelegate(this) );

    view()->installEventFilter(this);

    connect( lineEdit(), &QLineEdit::selectionChanged, lineEdit(), &QLineEdit::deselect);
    connect(view(), SIGNAL(pressed(QModelIndex)), this, SLOT(onItemPressed(QModelIndex)));
    connect(model_, SIGNAL(dataChanged(QModelIndex, QModelIndex, QVector<int>)), this, SLOT(updateText()));

    setModel( model_);
}

QStandardItem *CheckCombo::addItem(const QString &label, const QVariant &data, const Qt::CheckState checkState)
{
   QStandardItem* item = new QStandardItem(label);
   item->setCheckState(checkState);
   item->setData( data );
   item->setFlags( Qt::ItemIsUserCheckable | Qt::ItemIsEnabled);


   model_->appendRow(item);
   updateText();

   return item;
}

QVariantList CheckCombo::selectedDatas() const
{
    QVariantList selected;
    for (int i = 0; i < model_->rowCount(); i++)
    {
        if (model_->item(i)->checkState() == Qt::Checked)
        {
            selected.append( model_->item(i)->data() );
        }
    }
    return selected;
}

QList<int> CheckCombo::selectedInts() const
{
    QList<int> selected;
    for (int i = 0; i < model_->rowCount(); i++)
    {
        if (model_->item(i)->checkState() == Qt::Checked)
        {
            selected.append( model_->item(i)->data().toInt() );
        }
    }
    return selected;
}

void CheckCombo::haeMerkkaukset(const QDate &paivalle)
{
    KohdennusProxyModel merkkaukset(this, paivalle, -1, KohdennusProxyModel::MERKKKAUKSET);
    QVariantList lista = selectedDatas();

    clear();
    for(int i=0; i < merkkaukset.rowCount(); i++) {
        int koodi = merkkaukset.data( merkkaukset.index(i,0), KohdennusModel::IdRooli ).toInt();
        QString nimi = merkkaukset.data( merkkaukset.index(i,0), KohdennusModel::NimiRooli ).toString();
        addItem(nimi, koodi, Qt::Unchecked);
    }

    setSelectedItems( lista );
    updateText();
}

void CheckCombo::updateText()
{
    QString text;
    for (int i = 0; i < model_->rowCount(); i++)
    {
        if (model_->item(i)->checkState() == Qt::Checked)
        {
            if (!text.isEmpty())
            {
                text+= ", ";
            }

            text+= model_->item(i)->text();
        }
    }
    lineEdit()->setText(text);
}

void CheckCombo::setSelectedItems(const QList<int> &list)
{
    for(int i=0; i < model_->rowCount(); i++) {
        QStandardItem* item = model_->item(i);

        if( list.contains( model_->item(i)->data().toInt() ) )
            item->setCheckState( Qt::Checked );
        else
            item->setCheckState( Qt::Unchecked );
    }
    updateText();
}

void CheckCombo::setSelectedItems(const QVariantList &list)
{
    for(int i=0; i < model_->rowCount(); i++) {
        QStandardItem* item = model_->item(i);

        if( list.contains( model_->item(i)->data() ) )
            item->setCheckState( Qt::Checked );
        else
            item->setCheckState( Qt::Unchecked );
    }
    updateText();
}

void CheckCombo::onModelDataChanged()
{
    updateText();
}

void CheckCombo::onItemPressed(const QModelIndex &index)
{
    QStandardItem* item = model_->itemFromIndex(index);
    if( item->checkState() == Qt::Checked)
        item->setCheckState(Qt::Unchecked);
    else
        item->setCheckState(Qt::Checked);
    updateText();
}

bool CheckCombo::eventFilter(QObject *object, QEvent *event)
{
    if( object == lineEdit() && event->type() == QEvent::MouseButtonPress )
    {
        showPopup();
        return true;
    }
    if( object == view() && event->type() == QEvent::KeyPress)
    {
        QKeyEvent *keyEvent = static_cast<QKeyEvent*>(event);
        if( keyEvent->key() == Qt::Key_Enter ||
            keyEvent->key() == Qt::Key_Return)
        {
            focusNextChild();
            return true;
        }
    }

    return false;
}

void CheckCombo::focusOutEvent(QFocusEvent * event)
{
    QComboBox::focusOutEvent(event);
    updateText();
}

void CheckCombo::keyPressEvent(QKeyEvent *event)
{
    if( (event->key() == Qt::Key_Space || event->key() == Qt::Key_Return || event->key() == Qt::Key_Enter
         || event->key() == Qt::Key_Up || event->key() == Qt::Key_Down )
            && lineEdit()->hasFocus())
        showPopup();
    else
        QComboBox::keyPressEvent(event);
}


CheckCombo::CheckComboStyledItemDelegate::CheckComboStyledItemDelegate(QObject *parent) :
    QStyledItemDelegate (parent)
{

}

void CheckCombo::CheckComboStyledItemDelegate::paint(QPainter *painter_, const QStyleOptionViewItem &option_, const QModelIndex &index_) const
{
    QStyleOptionViewItem & refToNonConstOption = const_cast<QStyleOptionViewItem &>(option_);
    refToNonConstOption.showDecorationSelected = false;
    QStyledItemDelegate::paint(painter_, refToNonConstOption, index_);
}
