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

YksikkoCombo::YksikkoCombo(QWidget *parent, bool editable)
    : QComboBox(parent)
{
    setEditable(editable);

    QSortFilterProxyModel *proxy = new QSortFilterProxyModel(this);
    proxy->setSourceModel(&yksikot_);
    proxy->sort(0);
    setModel(proxy);

    connect( this, &QComboBox::editTextChanged, this, &YksikkoCombo::syotetty);
    connect( this, qOverload<int>(&QComboBox::currentIndexChanged), this, &YksikkoCombo::vaihtui );

    setUNkoodi("C62");

}

void YksikkoCombo::setYksikko(const QString &yksikko)
{
    yksikko_ = yksikko;
    unKoodi_ = "";

    setCurrentText(yksikko);
}

void YksikkoCombo::setUNkoodi(const QString &koodi)
{
    unKoodi_ = koodi;
    yksikko_ = yksikot_.nimi(koodi);
    setCurrentText(yksikko_);
    if( isEditable())
        lineEdit()->setText( yksikko_);
}

QString YksikkoCombo::yksikko() const
{
    return yksikko_;
}

QString YksikkoCombo::unKoodi() const
{
    return unKoodi_;
}

void YksikkoCombo::focusOutEvent(QFocusEvent *e)
{
    QComboBox::focusOutEvent(e);

    if( isEditable() )
        lineEdit()->setText( yksikko_ );
}

void YksikkoCombo::vaihtui(int indeksi)
{
    if( indeksi < 0)
        return;

    const QString koodi = model()->index(indeksi,0).data(YksikkoModel::UNKoodiRooli).toString();
    const QString nimi = model()->index(indeksi,0).data(Qt::DisplayRole).toString();

    unKoodi_ = koodi;
    yksikko_ = nimi;

    if( isEditable())
        lineEdit()->setText( yksikko_);

}

void YksikkoCombo::syotetty(const QString &teksti)
{
    if( yksikko_ != teksti && !teksti.isEmpty()) {
        yksikko_ = teksti;
        unKoodi_ = yksikot_.koodi(teksti);
    }
}

