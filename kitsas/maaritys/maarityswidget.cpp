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

#include "maarityswidget.h"
#include "db/kirjanpito.h"

#include <QLineEdit>
#include <QComboBox>
#include <QPlainTextEdit>

MaaritysWidget::MaaritysWidget(QWidget *parent) : QWidget(parent)
{

}

MaaritysWidget::~MaaritysWidget()
{

}

void MaaritysWidget::connectMuutokset()
{
    for(QWidget *widget : findChildren<QWidget*>() ) {

        QLineEdit *edit = qobject_cast<QLineEdit*>(widget);
        if( edit ) {
            connect( edit, &QLineEdit::textEdited, this, &MaaritysWidget::tarkastaMuokkaus );
            continue;
        }
        QComboBox *combo = qobject_cast<QComboBox*>(widget);
        if( combo ) {
            connect( combo, &QComboBox::currentTextChanged, this, &MaaritysWidget::tarkastaMuokkaus);
            continue;
        }
        QPlainTextEdit *ptedit = qobject_cast<QPlainTextEdit*>(widget);
        if( ptedit ) {
            connect( ptedit, &QPlainTextEdit::textChanged, this, &MaaritysWidget::tarkastaMuokkaus);
            continue;
        }
    }
}

void MaaritysWidget::tarkastaMuokkaus()
{
    bool muokattu = onkoMuokattu();
    emit tallennaKaytossa( muokattu );
}

