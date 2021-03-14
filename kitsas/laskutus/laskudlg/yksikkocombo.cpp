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
#include "yksikkocombo.h"

#include <QSortFilterProxyModel>
#include <QLineEdit>

#include <QDebug>

YksikkoCombo::YksikkoCombo(QWidget *parent)
    : QComboBox(parent)
{
    setEditable(true);

    QSortFilterProxyModel *proxy = new QSortFilterProxyModel(this);
    proxy->setSourceModel(&yksikot_);
    proxy->sort(0);
    setModel(proxy);

    connect( this, qOverload<int>(&QComboBox::currentIndexChanged),
             this, &YksikkoCombo::valittu);
    setCurrentIndex( findData("C62") );

}

void YksikkoCombo::setYksikko(const QString &yksikko)
{
    setCurrentIndex(-1);
    setEditText(yksikko);
}

void YksikkoCombo::setUNkoodi(const QString &koodi)
{
    setCurrentIndex( findData(koodi) );
}

QString YksikkoCombo::yksikko() const
{
    if( currentIndex() == -1) {
        return currentText();
    } else {
        return QString();
    }
}

QString YksikkoCombo::unKoodi() const
{
    return currentData(YksikkoModel::UNKoodiRooli).toString();
}

void YksikkoCombo::focusOutEvent(QFocusEvent *e)
{
    QComboBox::focusOutEvent(e);

    if( currentIndex() > -1)
        lineEdit()->setText( currentData(Qt::DisplayRole).toString() );
}

void YksikkoCombo::valittu()
{
    setEditText( currentData(Qt::DisplayRole).toString() );
}

