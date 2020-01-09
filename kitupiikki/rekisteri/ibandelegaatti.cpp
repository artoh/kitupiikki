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
#include "ibandelegaatti.h"
#include "laskutus/myyntilaskuntulostaja.h"

#include <QLineEdit>
#include "validator/ibanvalidator.h"

#include <QPainter>

IbanDelegaatti::IbanDelegaatti(QObject *parent) :
    QItemDelegate (parent)
{

}

void IbanDelegaatti::paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    painter->save();

    QString teksti = index.data(Qt::EditRole).toString();

    if( teksti.isEmpty()) {
        teksti = tr("Napsauta lisätäksesi tili");
    } else {
        teksti = MyyntiLaskunTulostaja::valeilla(teksti);
    }
    drawDisplay(painter, option, option.rect, teksti);
    drawFocus(painter, option, option.rect);
    painter->restore();

}

QWidget *IbanDelegaatti::createEditor(QWidget *parent, const QStyleOptionViewItem & /*option*/, const QModelIndex & /*index*/) const
{
    QLineEdit *edit = new QLineEdit(parent);
    IbanValidator *validator = new IbanValidator(parent);
    edit->setValidator(validator);
    edit->setPlaceholderText("IBAN-tilinumero");
    return edit;
}

void IbanDelegaatti::setEditorData(QWidget *editor, const QModelIndex &index) const
{
    QLineEdit *edit = qobject_cast<QLineEdit*>(editor);

    QString sisalto = index.data(Qt::EditRole).toString();
    if( IbanValidator::kelpaako(sisalto))
        edit->setText( sisalto );
}

void IbanDelegaatti::setModelData(QWidget *editor, QAbstractItemModel *model, const QModelIndex &index) const
{
    QLineEdit *edit = qobject_cast<QLineEdit*>(editor);
    if( IbanValidator::kelpaako( edit->text() ))
        model->setData(index, edit->text());
    else
        model->setData(index, QString());
}


